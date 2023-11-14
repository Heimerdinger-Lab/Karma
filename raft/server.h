#pragma once 
#include <co_context/all.hpp>
#include "common.h"
class server {
public:
    virtual ~server() {}
    virtual co_context::task<> start() = 0;
    virtual co_context::task<> abort() = 0;
    virtual void tick() = 0;
    virtual server_id id() const = 0;
};

void create_server();