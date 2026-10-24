An asynchronous BitTorrent (BEP 3) implementation written from scratch in C++20 on raw Linux syscalls. Bencode parsing, tracker communication, the peer wire protocol, piece management, non-blocking TCP networking, and piece-to-file mapping are implemented by hand without protocol libraries.

HTTP tracker requests are delegated to libcurl, and event-driven I/O is built on libev.