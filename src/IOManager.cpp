#include "../h/IOManager.hpp"

IOManager::IOManager()
{
    in_q_head = in_q_tail = out_q_head = out_q_tail = nullptr;
}

IOManager& IOManager::get()
{
    static IOManager instance;
    return instance;
}

bool IOManager::CircularBuffer::full()
{
    return front != -1 && front == back;
}

bool IOManager::CircularBuffer::empty()
{
    return front == -1;
}

void IOManager::CircularBuffer::push(char c)
{
    if(full()) panic("BAFER PUN");
    if(back == -1)
    {
        buf[0] = c;
        back = 1;
        front = 0;
    }
    else
    {
        buf[back++] = c;
        back = back % SZ;
    }
}

int IOManager::CircularBuffer::pop()
{
    if(back == -1)
    {
        return -1;
    }
    else
    {
        char c = buf[front++];
        front = front % SZ;
        if(front == back)
        {
            front = back = -1;
        }
        return c;
    }
}

void IOManager::putc(char c)
{
    // drain queue as far as possible
    // if successful & ready put else enqueue
    // if queue full add to IOWaitlist
    char* status_reg = reinterpret_cast<char*>(CONSOLE_STATUS);
    while((*status_reg & CONSOLE_TX_STATUS_BIT))
    {
        int ch = out_buf.pop();
        if(ch == -1)
            break;

        // if there were threads blocked on filling the buffer, unblock them
        // this should have been a semaphore but eh
        if(out_q_head)
        {
            out_buf.push((char)(out_q_head->extra_data));
            auto tmp = out_q_head;
            out_q_head = out_q_head->next;
            Scheduler::get().reschedule(tmp);
            if(out_q_head == nullptr) out_q_tail = nullptr;
        }
        
        *reinterpret_cast<char*>(CONSOLE_TX_DATA) = (char)(ch);
    }
    if((*status_reg & CONSOLE_TX_STATUS_BIT))
        *reinterpret_cast<char*>(CONSOLE_TX_DATA) = c;
    else
    {
        if(!out_buf.full())
        {
            out_buf.push(c);
        }
        else
        {
            // if buf full, block on put
            auto curr = Scheduler::get().deschedule_current();
            if(out_q_head == nullptr)
            {
                
                out_q_head = out_q_tail = curr;
            }
            else
            {
                out_q_tail->next = curr;
                out_q_tail = out_q_tail->next;
                out_q_tail->extra_data = (uint64)(c);
            }
        }
    }
}

void IOManager::getc()
{
    // drain queue as far as possible, drain reschedules(tight coupling!!!!)
    // if queue not empty fullful this and return
    // else add to IOWaitlist
    char* status_reg = reinterpret_cast<char*>(CONSOLE_STATUS);
    while(in_q_head)
    {
        if((*status_reg & CONSOLE_RX_STATUS_BIT) and !in_buf.full())
        {
            char ch = *reinterpret_cast<char*>(CONSOLE_RX_DATA);
            in_buf.push(ch);
        }
        if(in_buf.empty())
            break;

        in_q_head->data->ctx.x10 = (char)in_buf.pop();
        auto tmp = in_q_head;
        in_q_head = in_q_head->next;
        Scheduler::get().reschedule(tmp);
        if(in_q_head == nullptr) in_q_tail = nullptr;
    }
    while((*status_reg & CONSOLE_RX_STATUS_BIT) and !in_buf.full())
    {
        char ch = *reinterpret_cast<char*>(CONSOLE_RX_DATA);
        in_buf.push(ch);
    }
    if((!in_q_head) and (!in_buf.empty())) // sus condition
    {
        Scheduler::Running->ctx.x10 = (char)(in_buf.pop());
    }
    else
    {
        // if buf empty, block on get
        auto curr = Scheduler::get().deschedule_current();
        if(in_q_head == nullptr)
        {
            
            in_q_head = in_q_tail = curr;
        }
        else
        {
            in_q_tail->next = curr;
            in_q_tail = in_q_tail->next;
        }
    }
}

void IOManager::interrupt()
{
    // realno je trebalo da delove iz getc i putc refaktorisem u fill_in() i drain_out()
    // i da to ponovo iskoristim ovde......
    // ma kad se kod bude menjao bice refaktorisan u smisleniji oblik, za sad neka ga
    char* status_reg = reinterpret_cast<char*>(CONSOLE_STATUS);
    bool flag;
    do
    {
        char stat = *status_reg;
        flag = false;

        if((stat & CONSOLE_RX_STATUS_BIT) and !in_buf.full())
        {
            flag = true;
            char ch = *reinterpret_cast<char*>(CONSOLE_RX_DATA);
            in_buf.push(ch);

            if(in_q_head)
            {
                in_q_head->data->ctx.x10 = (char)in_buf.pop();
                auto tmp = in_q_head;
                in_q_head = in_q_head->next;
                Scheduler::get().reschedule(tmp);
                if(in_q_head == nullptr) in_q_tail = nullptr;
            }
        }

        if((stat & CONSOLE_TX_STATUS_BIT) and !out_buf.empty())
        {
            flag = true;
            int ch = out_buf.pop();
            *reinterpret_cast<char*>(CONSOLE_TX_DATA) = (char)(ch);
            
            if(out_q_head)
            {
                out_buf.push((char)(out_q_head->extra_data));
                auto tmp = out_q_head;
                out_q_head = out_q_head->next;
                Scheduler::get().reschedule(tmp);
                if(out_q_head == nullptr) out_q_tail = nullptr;
            } 
        }


    } while(flag);
}

void IOManager::drain_buffer()
{
    char* status_reg = reinterpret_cast<char*>(CONSOLE_STATUS);
    while(!out_buf.empty())
    {
        while(not (*status_reg & CONSOLE_TX_STATUS_BIT)){;}
        *reinterpret_cast<char*>(CONSOLE_TX_DATA) = out_buf.pop();
    }
}