//
// Created by stefangliga on 29/04/23.
//

#include "../lib/hw.h"
#include "util.hpp"

#pragma once

uint32 hash(uint32 x);

/*
 * Osnovni princip:
 * Na pocetku heap-a rezervisemo segment gde za svaki blok memorije cuvamo 4-bitni deskriptor
 * Deskriptori se cuvaju po 2 spakovana u bajt, gde prvi ide u nizi nibble a drugi u visi
 * Deskriptor ima znacenje velicine segmenta alociranog sa pocetkom na odgovarajucem bloku
 * Velicine >=15 se kodiraju tako sto se u deskriptor zapise 1111 pa se zatim deskriptori sledecih blokova hijack-uju
 * Konkretno sledecih 8-9 deskriptora, 1 preskacemo za slucaj da smo procitali nizi nibble
 * i u sledecih 8 pakujemo 4bajtnu velicinu.
 * Ustvari za zadata ogranicenja bi bila dosta 3bajtna velicina
 * ali nas ne kosta prakticno nista da budemo fleksibilni i ostavimo mesta za eventualne nadogradje hardvera.
 * Pri oslobadjanu se radi spajanje slobodnog prostora SAMO SA DESNA
 * U OOM situaciji, kao poslednja nada radi se O(n) prolazak kroz celu free listu
 * i na svakom segmentu se radi spajanje s desna, za slucaj da je sablon alokacije takav
 * da veliki broj segmenata ima slobodne segmente s leva(posto je samo merge sa desna jeftin... i implementiran)
 * Rationale:
 * Gledano je da se minimizuje worst case memorijski overhead.
 * Naime naivna tehnika zaglavlja pre alokacije ima worst case overhead od 50%, za male alokacije(1/1+1)
 * Ova tehnika ima fisni memorijski overhead od malo preko 4bit/BS ~ 0.78125% za BS=64.
 * Racunica nije tacna zbog zaokruzivanja velicine deskriptorskog segmenta na pun blok
 *
 * Ovo je pregled struktura, za detalje logike videti komentar u .cpp fajlu
 */
class Allocator {
public:
    Allocator();

    char* allocate(unsigned blocks);
    void free(char* addr);

    static Allocator& get();

    uint32 dumpdesc(char* d) {return read_descriptor(d);}

private:
    struct FreeListNode
    {
        uint32 size;
        uint32 magic; // 1 od 2 nivoa zastite od heap buffer overflow
        FreeListNode* next;
        FreeListNode* prev;
        uint32 size_hash; // 2 nivo zastite od heap buffer overflow
        constexpr static uint32 MAGIC = 0xC0FFEE20;

        static FreeListNode* construct_at(char* addr, uint32 sz, FreeListNode* nxt, FreeListNode* prv);
        void validate() const {if(magic != MAGIC or hash(size) != size_hash) panic("HEAP OVERFLOW");}
        void destruct() {size = 0; magic = 0x159177ED; next = nullptr; prev = nullptr; size_hash = 0;}
        void setsize(uint32 sz) {size = sz; size_hash = hash(sz);}
    };
    char* descriptors;
    char* real_heap_start;
    FreeListNode* head;
    FreeListNode* skip;
    uint32 skip_alloc_size;

    uint32 read_descriptor(const char* block) const;
    void   write_descriptor(const char* block, uint32 size);
    void   erase_descriptor(const char* block);
    static void validate_descriptor_re(uint8 desc); //re=read+erase
    static void validate_descriptor_w(uint8 desc, bool hi_n, bool big);

    char* cut_block(FreeListNode* pNode, unsigned int blocks);

    void try_coalesce(FreeListNode* pNode);
};

