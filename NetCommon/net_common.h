#pragma once
//common includes
#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <optional>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifdef  _WIN32
#define _WINT32_WINNT 0x0A00 //Windows 10 onwards
#endif //  _WIN32

//asio
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>




