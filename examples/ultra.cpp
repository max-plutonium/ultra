//! [function_task]
int some_function(std::string);
...
auto ptask = std::make_shared<ultra::function_task<int (std::string)>>(1, some_function, "123");
std::future<int> future = ptask->get_future();
...
thread_pool.execute(std::move(ptask));
//! [function_task]
