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
#ifndef ASTRA_POINT_HPP
#define ASTRA_POINT_HPP

#include "../../astra_core/astra_core.hpp"
#include "../capi/astra_ctypes.h"
#include "../capi/streams/point_capi.h"
#include "Image.hpp"

namespace astra {

    class PointStream : public DataStream
    {
    public:
        PointStream()
        {}

        explicit PointStream(astra_streamconnection_t connection)
            : DataStream(connection)
        {
            pointStream_ = reinterpret_cast<astra_pointstream_t>(connection);
        }

        static const astra_stream_type_t id = ASTRA_STREAM_POINT;

    private:
        astra_pointstream_t pointStream_;
    };

    class PointFrame : public ImageFrame<Vector3f, ASTRA_STREAM_POINT>
    {
    public:
        PointFrame(astra_imageframe_t frame)
            : ImageFrame(frame, ASTRA_PIXEL_FORMAT_POINT)
        {}
    };
}

#endif // ASTRA_POINT_HPP
