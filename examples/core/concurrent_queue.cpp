//! [enqueue]
concurrent_queue<std::string, std::mutex> queue;
...
queue.enqueue("123");
//! [enqueue]

//! [dequeue]
concurrent_queue<std::string, std::mutex> queue;
...
std::string str;
queue.dequeue(str);
//! [dequeue]