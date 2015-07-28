//! [push]
concurrent_queue<std::string, std::mutex> queue;
...
queue.push("123");
//! [push]

//! [pull]
concurrent_queue<std::string, std::mutex> queue;
...
std::string str;
queue.pull(str);
//! [pull]

//! [wait_pull]
concurrent_queue<std::string, std::mutex> queue;
...
std::string str;
queue.wait_pull(str);
//! [wait_pull]
