// Aqsis
// Copyright (C) 2001, Paul C. Gregory and the other authors and contributors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the software's owners nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// (This is the New BSD license)

#include "varspec.h"

namespace Aqsis {

namespace Stdvar
{
#   define DEFINE_STD_VAR(type, count, name)                          \
        const VarSpec name(VarSpec::type, count, ustring(#name))

    DEFINE_STD_VAR(Float, 1, alpha);
    DEFINE_STD_VAR(Color, 1, Ci);
    DEFINE_STD_VAR(Color, 1, Cl);
    DEFINE_STD_VAR(Color, 1, Cs);
    DEFINE_STD_VAR(Vector, 1, dPdu);
    DEFINE_STD_VAR(Vector, 1, dPdv);
    DEFINE_STD_VAR(Float, 1, du);
    DEFINE_STD_VAR(Float, 1, dv);
    DEFINE_STD_VAR(Point, 1, E);
    DEFINE_STD_VAR(Vector, 1, I);
    DEFINE_STD_VAR(Vector, 1, L);
    DEFINE_STD_VAR(Float, 1, ncomps);
    DEFINE_STD_VAR(Normal, 1, Ng);
    DEFINE_STD_VAR(Normal, 1, Ns);
    DEFINE_STD_VAR(Normal, 1, N);
    DEFINE_STD_VAR(Color, 1, Oi);
    DEFINE_STD_VAR(Color, 1, Ol);
    DEFINE_STD_VAR(Color, 1, Os);
    DEFINE_STD_VAR(Point, 1, P);
    DEFINE_STD_VAR(Point, 1, Ps);
    DEFINE_STD_VAR(Float, 1, s);
    DEFINE_STD_VAR(Float, 1, t);
    DEFINE_STD_VAR(Float, 1, time);
    DEFINE_STD_VAR(Float, 1, u);
    DEFINE_STD_VAR(Float, 1, v);

    DEFINE_STD_VAR(Float, 2, st);
    DEFINE_STD_VAR(Float, 1, z);

#undef DEFINE_STD_VAR
}


} // namespace Aqsis
