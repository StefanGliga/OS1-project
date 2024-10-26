//
// Created by stefangliga on 29/04/23.
//

#include "../h/Allocator.hpp"

// score = 282.56087830082657
uint32 hash(uint32 x)
{
    x += x << 5;
    x += x << 8;
    x ^= x >> 14;
    x += x << 2;
    x += x << 10;
    x ^= x >> 13;
    return x;
}

Allocator::FreeListNode*
Allocator::FreeListNode::construct_at(char* addr, uint32 sz, Allocator::FreeListNode* nxt, Allocator::FreeListNode* prv)
{
    auto dis = (FreeListNode*)(addr);
    dis->size = sz;
    dis->next = nxt;
    dis->prev = prv;
    dis->magic = MAGIC;
    dis->size_hash = hash(sz);
    return dis;
}

Allocator& Allocator::get()
{
    static Allocator instance;
    return instance;
}

Allocator::Allocator(): skip{nullptr}, skip_alloc_size(-1)
{
    /*
     * S = HEAP_START
     * R = REAL_HEAP_START
     * E = HEAP_END
     * B = BLOCK_SIZE
     *
     * interpolate the ratio of 0.5:B, equivalent to 1:2B
     * R = 2B*S + E / (2B + 1)
     */
    const auto E = reinterpret_cast<uint64>(HEAP_END_ADDR);
    const auto S = reinterpret_cast<uint64>(HEAP_START_ADDR);
    auto tmp = (2*MEM_BLOCK_SIZE*S + E + 2*MEM_BLOCK_SIZE ) / (1 + 2*MEM_BLOCK_SIZE);
    auto res = tmp % MEM_BLOCK_SIZE; // ovo sam mogao i bitskim operatorima ali ajde da se osiguram da radi i kad BS nije stepen 2
    if(res != 0)
    {
        tmp += MEM_BLOCK_SIZE - res;
    }
    real_heap_start = reinterpret_cast<char*>(tmp);

    for(auto it = (uint64 *)HEAP_START_ADDR; it < (uint64 *)real_heap_start; it++)
        *it = 0;

    descriptors = (char*)(HEAP_START_ADDR);
    head = reinterpret_cast<FreeListNode*>(real_heap_start);
    FreeListNode::construct_at(real_heap_start, (E-tmp)/MEM_BLOCK_SIZE, nullptr, nullptr);
}

/*
 * Osnovni princip:
 * FreeList allocator sa par izmena
 * Freelist allocator cuva 2 pokazivaca umesto 1, head i pokazivac na blok posle poslednjeg dodeljenog(skip)
 * Takodje se cuva velicina poslednjeg dodeljenog bloka
 * Time se ubrzava situacija u kojoj se alocira vise velikih blokova
 * u slucaju kada se veliki broj malih blokova nagomila pri glavi freelist-e
 * Posto znamo da nigde izmedju glave i sredine nema blokova manjih od poslednje alokacija
 * Ovo je veoma gruba aproksimacija skiplist-e
 * Pri oslobadjanju se invalidira skip
 */
char* Allocator::allocate(unsigned blocks)
{
    FreeListNode* iter;
    if(blocks>skip_alloc_size and skip)
        iter = skip;
    else
        iter = head;

    if(not iter) return nullptr; // HARD OOM!
    while(iter and iter->size < blocks)
    {
        iter->validate();
        if(read_descriptor(reinterpret_cast<char*>(iter)) != 0) panic("NESLOBODAN BLOK U FREE LISTI");
        iter = iter->next;
    }
    if(iter)
    {
        iter->validate();
        if(read_descriptor(reinterpret_cast<char*>(iter)) != 0) panic("NESLOBODAN BLOK U FREE LISTI");
        skip_alloc_size = blocks;
        auto const prev = iter->prev;
        auto const ret = cut_block(iter, blocks); // updates freelist and returns ptr to now allocated mem
        if(prev)
        {
            skip = prev->next;
        }
        else
        {
            skip_alloc_size=-1;
            skip=nullptr; // dohvatamo pomereni deskriptor indirektno preko prev. Ako prev ne postoji, skiplista nema smisla i resetuje se
        }
        return ret;
    }

    // LAST DITCH ATTEMPT, O(n) coalescing svega pa ako i to ne uspe, OOM
    skip_alloc_size = -1;
    skip = nullptr;
    iter = head;
    while(iter)
    {
        try_coalesce(iter);
        iter = iter->next;
    }
    iter = head;
    // code duplication, ew, but it is what it is
    while(iter and iter->size < blocks)
    {
        iter->validate();
        if(read_descriptor(reinterpret_cast<char*>(iter)) != 0) panic("NESLOBODAN BLOK U FREE LISTI");
        iter = iter->next;
    }
    if(iter)
    {
        iter->validate();
        if(read_descriptor(reinterpret_cast<char*>(iter)) != 0) panic("NESLOBODAN BLOK U FREE LISTI");
        skip_alloc_size = blocks;
        auto const prev = iter->prev;
        auto const ret = cut_block(iter, blocks); // updates freelist and returns ptr to now allocated mem
        if(prev)
        {
            skip = prev->next;
        }
        else
        {
            skip_alloc_size=-1;
            skip=nullptr; // dohvatamo pomereni deskriptor indirektno preko prev. Ako prev ne postoji, skiplista nema smisla i resetuje se
        }
        return ret;
    }

    return nullptr;
}

void Allocator::free(char* addr)
{
    if(addr == nullptr)
        return;

    skip_alloc_size = -1;
    skip = nullptr;
    uint size = read_descriptor(addr);
    erase_descriptor(addr);
    if(size>skip_alloc_size)
    {
        skip_alloc_size = -1;
        skip = nullptr;
    }
    head = FreeListNode::construct_at(addr, size, head, nullptr);
    if(head->next->prev) panic("CVOR USRED LISTE JE BIO HEAD");
    head->next->prev = head;

    return;
}

auto const errormsg = "NEPORAVNAT POKAZIVAC JE ZAVRSIO U KERNEL ALLOCATOR";


uint32 Allocator::read_descriptor(const char* block) const
{
    uint32 block_id = block - real_heap_start;
    if(block_id % MEM_BLOCK_SIZE != 0)
    {
        panic(errormsg);
    }
    block_id = block_id / MEM_BLOCK_SIZE;
    uint32 index = block_id / 2;
    uint32 hi_nibble = block_id % 2; // compiler will optimize into bitwise ops
    uint8 res = descriptors[index];
    validate_descriptor_re(res);
    if(hi_nibble)
    {
        res >>= 4;
    }
    else
    {
        res &= 0xF;
    }
    if(res != 0xF)
    {
        return res;
    }
    else
    {
        return *reinterpret_cast<uint32*>(descriptors+index+1);
    }

}

void Allocator::write_descriptor(const char* block, uint32 size)
{
    // krsim pravila programiranja i dupliciram kod kako bih mogao da ponovo iskoristim medjukorake
    uint32 block_id = block - real_heap_start;
    if(block_id % MEM_BLOCK_SIZE != 0)
    {
        panic(errormsg);
    }
    block_id = block_id / MEM_BLOCK_SIZE;
    uint32 index = block_id / 2;
    uint32 hi_nibble = block_id % 2; // compiler will optimize into bitwise ops
    uint8 res = descriptors[index];
    uint8 desc_cached = res;
    validate_descriptor_re(res);
    if(hi_nibble)
    {
        res >>= 4;
    }
    else
    {
        res &= 0xF;
    }
    if(res != 0)
    {
        panic("GAZENJE PO VEC POSTOJECEM SEGMENTU");
    }
    // TODO: razmotriti guranje ova da 2 if-a u validate funkciju
    validate_descriptor_w(desc_cached, hi_nibble, size>1);
    if(size<0xF)
    {
        uint8 tmp = size << (4*hi_nibble);
        descriptors[index] |= tmp;
    }
    else
    {
        uint8 tmp = 0xF << (4*hi_nibble);
        descriptors[index] |= tmp;
        if(*reinterpret_cast<uint32*>(descriptors+index+1) != 0)
        {
            panic("VEC POSTOJAO SEGMENT USRED NOVOALOCIRANOG SEGMENTA");
        }
        *reinterpret_cast<uint32*>(descriptors+index+1) = size;
    }
}

void Allocator::erase_descriptor(const char* block)
{
    uint32 block_id = block - real_heap_start;
    if(block_id % MEM_BLOCK_SIZE != 0)
    {
        panic(errormsg);
    }
    block_id = block_id / MEM_BLOCK_SIZE;
    uint32 index = block_id / 2;
    uint32 hi_nibble = block_id % 2; // compiler will optimize into bitwise ops
    uint8 res = descriptors[index];
    validate_descriptor_re(res);
    if(hi_nibble)
    {
        res >>= 4;
    }
    else
    {
        res &= 0xF;
    }
    if(res == 0)
    {
        panic("OBRISAN VEC PRAZAN DESKRIPTOR");
    }
    uint8 tmp;
    if(hi_nibble)
    {
        tmp = 0x0F; // tamper hi, keep low
    }
    else
    {
        tmp = 0xF0; // tamper low, keep hi
    }
    descriptors[index] &=  tmp;
    if(res == 0xF)
    {
        *reinterpret_cast<uint32*>(descriptors+index+1) = 0;
    }
}

inline void Allocator::validate_descriptor_re(uint8 desc)
{
    if(desc & 0xF0)
    {
        if((desc & 0X0F) > 1)
        {
            panic("BESMISLENI DESKRIPTOR, UOCENI PREKLAPAJUCI SEGMENTI");
        }
    }
}
inline void Allocator::validate_descriptor_w(uint8 desc, bool hi_n, bool big) // hteo sam prvo da pakujem sve u 1 char, ali reko ne vredi overheada da se spakuje u pozivu
{
    // najnizi bit - 0 low 1 high  nibble
    // ako je ista drugo postavljeno to je znak da je size>1
    if(hi_n)
    {
        if((desc & 0x0F) > 1)
        {
            // 1 nereg situacija: ovaj blok pripada segmentu koju pocinje 1 blok pre
            // logika: pisemo u gornji nibble, u donjem je velicina>1
            panic("ALOKACIJA SEGMENTA BI STVORILA PREKLAPANJE");
        }
    }
    else
    {
        if(desc & 0XF0)
        {
            if(big)
            {
                // 2 nereg situacija: neki 'nasi' blokovi pripadaju segmentu posle nas
                // logika: pisemo u donji nibble, u gornjem ima nesto, mi smo velicine>1
                panic("ALOKACIJA SEGMENTA BI STVORILA PREKLAPANJE");
            }
        }
    }
}

// returns now allocated memory, updates freelist
char* Allocator::cut_block(Allocator::FreeListNode* pNode, unsigned int blocks)
{
    char*         const adr     = reinterpret_cast<char*>(pNode);
    FreeListNode* const next    = pNode->next;
    FreeListNode* const prev    = pNode->prev;
    uint32        const newsize = pNode->size - blocks;
    char*         const newadr  = adr + blocks * MEM_BLOCK_SIZE;

    pNode->destruct();
    write_descriptor(adr, blocks); // core of allocation
    /*
     * Logic
     * destroy node and move it elsewhere
     * if new_bs is 0, it's not a move it's a deletion, relink prev and next
     * if next doesn't exist, ignore any updates
     * if prev doesn't exist, update head
     * Temp variables used to avoid nested if
     */
    FreeListNode* var1;
    FreeListNode* var2;
    if(newsize == 0)
    {
        var1 = prev;
        var2 = next;
    }
    else {
        var1 = var2 = FreeListNode::construct_at(newadr, newsize, next, prev);
    }

    if(next) next->prev = var1;
    if(prev) prev->next = var2;
    else Allocator::head = var2;

    return adr;
}

void Allocator::try_coalesce(Allocator::FreeListNode* pNode)
{
    while(true)
    {
        uint32 size = pNode->size;
        auto addr = reinterpret_cast<char*>(pNode) + size * MEM_BLOCK_SIZE;
        if(addr >= HEAP_END_ADDR) return;
        if(read_descriptor(addr) != 0)
            return;
        // next (in memory) seg is also a free seg, coalesce

        auto const neighbor = reinterpret_cast<FreeListNode*>(reinterpret_cast<char*>(pNode) + size * MEM_BLOCK_SIZE);
        neighbor->validate();
        FreeListNode* const next     = neighbor->next;
        FreeListNode* const prev     = neighbor->prev;
        uint32        const n_size   = neighbor->size;
        pNode->setsize(size+n_size);
        if(next) next->prev = prev;
        if(prev) prev->next = next;
        else Allocator::head = next;
    }
}

