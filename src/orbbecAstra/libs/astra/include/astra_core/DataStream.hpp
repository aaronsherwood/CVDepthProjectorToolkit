// This file is part of the Orbbec Astra SDK [https://orbbec3d.com]
// Copyright (c) 2015 Orbbec 3D
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Be excellent to each other.
#ifndef ASTRA_DATASTREAM_HPP
#define ASTRA_DATASTREAM_HPP

#include "capi/astra_core.h"
#include "StreamDescription.hpp"
#include <stdexcept>

namespace astra {

    class DataStream
    {
    public:
        DataStream()
        {}

        DataStream(astra_streamconnection_t connection)
            : connection_(connection)
        {
            if(connection_ != nullptr)
            {
                astra_stream_get_description(connection,&description_);
            }
        }

        bool is_available() { return connection_ != nullptr; }

        void start()
        {
            if(connection_ == nullptr)
            {
                throw std::logic_error("Cannot start a stream that is not available");
            }
            astra_stream_start(connection_);
        }
        void stop()

        {
            if(connection_ == nullptr)
            {
                throw std::logic_error("Cannot stop a stream that is not available");
            }
            astra_stream_stop(connection_);
        }

    private:
        astra_streamconnection_t connection_{nullptr};
        astra_stream_desc_t description_;
    };
}

#endif // ASTRA_DATASTREAM_HPP
