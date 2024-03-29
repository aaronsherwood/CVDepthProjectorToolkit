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
#ifndef ASTRA_PLUGIN_STREAM_BIN_HPP
#define ASTRA_PLUGIN_STREAM_BIN_HPP

#include "../capi/astra_types.h"
#include "../capi/plugins/astra_plugin.h"
#include "PluginServiceProxy.hpp"

namespace astra { namespace plugins {

    template<typename TFrameType>
    class stream_bin
    {
    public:
        stream_bin(PluginServiceProxy& pluginService,
                   astra_stream_t streamHandle,
                   size_t dataSize)
            : streamHandle_(streamHandle),
              pluginService_(pluginService)
        {
            size_t dataWrapperSize = dataSize + sizeof(TFrameType);
            pluginService_.create_stream_bin(streamHandle,
                                             dataWrapperSize,
                                             &binHandle_,
                                             &currentBuffer_);
        }

        ~stream_bin()
        {
            pluginService_.destroy_stream_bin(streamHandle_, &binHandle_, &currentBuffer_);
        }

        bool has_connections()
        {
            bool hasConnections = false;
            pluginService_.bin_has_connections(binHandle_, &hasConnections);

            return hasConnections;
        }

        void cycle()
        {
            pluginService_.cycle_bin_buffers(binHandle_, &currentBuffer_);
        }

        TFrameType* begin_write(size_t frameIndex)
        {
            if (locked_)
                return reinterpret_cast<TFrameType*>(currentBuffer_->data);

            locked_ = true;
            currentBuffer_->frameIndex = frameIndex;
            return reinterpret_cast<TFrameType*>(currentBuffer_->data);
        }

        std::pair<astra_frame_t*, TFrameType*> begin_write_ex(size_t frameIndex)
        {
            if (locked_)
            {
                return std::make_pair(currentBuffer_, reinterpret_cast<TFrameType*>(currentBuffer_->data));
            }

            locked_ = true;
            currentBuffer_->frameIndex = frameIndex;

            return std::make_pair(currentBuffer_, reinterpret_cast<TFrameType*>(currentBuffer_->data));
        }

        void end_write()
        {
            if (!locked_)
                return;

            cycle();
            locked_ = false;
        }

        void link_connection(astra_streamconnection_t connection)
        {
            pluginService_.link_connection_to_bin(connection, binHandle_);

        }

        void unlink_connection(astra_streamconnection_t connection)
        {
            pluginService_.link_connection_to_bin(connection, nullptr);
        }

    private:
        astra_stream_t streamHandle_;
        astra_bin_t binHandle_;
        size_t bufferSize_{0};
        astra_frame_t* currentBuffer_{nullptr};
        PluginServiceProxy& pluginService_;
        bool locked_{false};
    };

}}

#endif /* ASTRA_PLUGIN_STREAM_BIN_HPP */
