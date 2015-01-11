//! [pool]
class hello_task : public core::task
{
    void run() override
    {
        std::cout << "Hello world!" << std::endl;
    }
}

auto hello = std::make_shared<hello_task>;
thread_pool pool;
pool.execute(hello);
//! [pool]
