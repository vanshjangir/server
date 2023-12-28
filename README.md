# Client-Server message sharing
The idea is to handle multiple live, concurrent connections on a single machine with minimum CPU usage. It does that by using thread pooling and epoll(linux specific).
