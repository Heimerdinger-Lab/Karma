#pragma once

namespace client {
enum client_error {
    rpc_timeout,
    send_error,
    connection_error,
    frame_error,
};
};