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

/// \file Renderer interface implementation
/// \author Chris Foster

#include <fstream>
#include <stack>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <aqsis/riutil/errorhandler.h>
#include <aqsis/riutil/ribparser.h>
#include <aqsis/riutil/ricxx.h>
#include <aqsis/riutil/ricxxutil.h>
#include <aqsis/riutil/ricxx_filter.h>
#include <aqsis/riutil/tokendictionary.h>
#include <aqsis/util/exception.h>

#include "displaymanager.h"
#include "renderer.h"
#include "surfaces.h"
#include "util.h"

namespace Aqsis {

namespace {

VarSpec typeSpecToVarSpec(const Ri::TypeSpec& spec)
{
    VarSpec out;
    switch(spec.type)
    {
        case Ri::TypeSpec::Float:  out.type = VarSpec::Float;  break;
        case Ri::TypeSpec::Point:  out.type = VarSpec::Point;  break;
        case Ri::TypeSpec::Color:  out.type = VarSpec::Color;  break;
        case Ri::TypeSpec::String: out.type = VarSpec::String; break;
        case Ri::TypeSpec::Vector: out.type = VarSpec::Vector; break;
        case Ri::TypeSpec::Normal: out.type = VarSpec::Normal; break;
        case Ri::TypeSpec::HPoint: out.type = VarSpec::Hpoint; break;
        case Ri::TypeSpec::Matrix: out.type = VarSpec::Matrix; break;
        case Ri::TypeSpec::Integer:
        case Ri::TypeSpec::MPoint:
        case Ri::TypeSpec::Unknown:
            AQSIS_THROW_XQERROR(XqValidation, EqE_Bug,
                                "No VarSpec for TypeSpec type");
    }
    out.arraySize = spec.arraySize;
    return out;
}

/// An error handler which just sends errors to stderr
///
/// (TODO: Make the errors go to a user-specified error handler?)
class PrintErrorHandler : public Ri::ErrorHandler
{
    public:
        explicit PrintErrorHandler(ErrorCategory verbosity = Debug)
            : Ri::ErrorHandler(verbosity)
        { }

    protected:
        virtual void sendError(int code, const std::string& message)
        {
            std::ostream& out = std::cerr;
            switch(errorCategory(code))
            {
                case Debug:   out << "\033[32m"   "DEBUG: "   ; break;
                case Info:    out <<              "INFO: "    ; break;
                case Warning: out << "\033[36m"   "WARNING: " ; break;
                case Error:   out << "\033[1;31m" "ERROR: "   ; break;
                case Severe:  out << "\033[1;31m" "SEVERE: "  ; break;
                default: break;
            }
            out << message << "\033[0m" << std::endl;
        }
};

class ApiServices : public Ri::RendererServices
{
    public:
        ApiServices();

        RtVoid declare(RtConstString name, RtConstString declaration)
        {
            m_tokenDict.declare(name, declaration);
        }

        //--------------------------------------------------
        // from Ri::RenderServices
        virtual Ri::ErrorHandler& errorHandler()
        {
            return m_errorHandler;
        }

        virtual RtFilterFunc getFilterFunc(RtConstToken name) const
        {
            // TODO: Something less crap!
            if(!strcmp(name, "box"))      return (RtFilterFunc)1;
            if(!strcmp(name, "gaussian")) return (RtFilterFunc)2;
            if(!strcmp(name, "sinc"))     return (RtFilterFunc)3;
            if(!strcmp(name, "disc"))     return (RtFilterFunc)4;
            return 0;
            //return getFilterFuncByName(name);
        }
        virtual RtConstBasis* getBasis(RtConstToken name) const
        {
            return 0;
            //return getBasisByName(name);
        }
        virtual RtErrorFunc getErrorFunc(RtConstToken name) const
        {
            return 0;
            //return getErrorFuncByName(name);
        }
        virtual RtProcSubdivFunc getProcSubdivFunc(RtConstToken name) const
        {
            return 0;
            //return getProcSubdivFuncByName(name);
        }

        virtual Ri::TypeSpec getDeclaration(RtConstToken token,
                                        const char** nameBegin = 0,
                                        const char** nameEnd = 0) const
        {
            return m_tokenDict.lookup(token, nameBegin, nameEnd);
        }

        virtual Ri::Renderer& firstFilter()
        {
            if(!m_filterChain.empty())
                return *m_filterChain.back();
            return *m_api;
        }

        virtual void addFilter(const char* name,
                               const Ri::ParamList& filterParams
                               = Ri::ParamList())
        {
            boost::shared_ptr<Ri::Renderer> filter;
            filter.reset(createFilter(name, *this, firstFilter(),
                                      filterParams));
            if(filter)
                m_filterChain.push_back(filter);
            else
            {
                AQSIS_THROW_XQERROR(XqValidation, EqE_BadToken,
                        "filter \"" << name << "\" not found");
            }
        }

        virtual void parseRib(std::istream& ribStream, const char* name,
                              Ri::Renderer& context)
        {
            if(!m_parser)
                m_parser.reset(RibParser::create(*this));
            m_parser->parseStream(ribStream, name, context);
        }
        using Ri::RendererServices::parseRib;

    private:
        /// Renderer API
        boost::shared_ptr<Ri::Renderer> m_api;
        /// Parser for ReadArchive.  May be NULL (created on demand).
        boost::shared_ptr<RibParser> m_parser;
        /// Chain of filters
        std::vector<boost::shared_ptr<Ri::Renderer> > m_filterChain;
        /// Error handler.
        PrintErrorHandler m_errorHandler;
        /// Declared tokens
        TokenDict m_tokenDict;
};


//------------------------------------------------------------------------------
/// Class holding camera information supplied through the interface.
class CameraInfo
{
    public:
        enum Type {
            Orthographic,
            Perspective,
            UserDefined
        };

        /// Construct camera with default parameters
        CameraInfo()
            : m_type(Orthographic),
            m_fov(90),
            m_userFrameAspect(false),
            m_userScreenWindow(false),
            m_left(-4.0f/3.0f),
            m_right(4.0f/3.0f),
            m_bottom(-1.0f),
            m_top(1.0f)
        { }

        /// Set the camera type
        void setType(Type type) { m_type = type; }
        /// Set the field of view for the perspective camera type
        void setFov(float fov)  { m_fov = fov; }

        /// Set the screen window
        void setScreenWindow(float left, float right, float bottom, float top)
        {
            m_userScreenWindow = true;
            m_left = left;
            m_right = right;
            m_bottom = bottom;
            m_top = top;
        }

        /// Set the frame aspect ratio.
        void setFrameAspect(float aspect, bool setByUser)
        {
            if(m_userScreenWindow)
                return;
            if(m_userFrameAspect && !setByUser)
                return;
            m_userFrameAspect = setByUser;
            if(aspect >= 1)
            {
                m_left = -aspect;
                m_right = aspect;
                m_bottom = -1;
                m_top = 1;
            }
            else
            {
                m_left = -1;
                m_right = 1;
                m_bottom = -1/aspect;
                m_top = 1/aspect;
            }
        }

        /// Get the camera->screen matrix specified by this camera info.
        Mat4 camToScreenMatrix(const Options& opts) const
        {
            Mat4 proj;
            switch(m_type)
            {
                case Orthographic:
                    proj = orthographicProjection(opts.clipNear, opts.clipFar);
                    break;
                case Perspective:
                    proj = perspectiveProjection(m_fov, opts.clipNear, opts.clipFar);
                    break;
                case UserDefined:
                    // TODO!
                    break;
            }
            return proj * screenWindow(m_left, m_right, m_bottom, m_top);
        }

    private:
        Type m_type; ///< Camera type
        float m_fov; ///< field of view for perspective cameras
        bool m_userFrameAspect;  ///< True if user set the frame aspect ratio
        bool m_userScreenWindow; ///< True if user set the screen window
        float m_left, m_right, m_bottom, m_top; ///< ScreenWindow parameters
};


//------------------------------------------------------------------------------
/// Class managing the transformation stack
class TransformStack
{
    public:
        TransformStack()
            : m_transforms()
        {
            m_transforms.push(Mat4());
        }

        void push()
        {
            m_transforms.push(m_transforms.top());
        }
        void pop()
        {
            m_transforms.pop();
        }
        const Mat4& top() const
        {
            return m_transforms.top();
        }

        void concat(const Mat4& trans)
        {
            m_transforms.top() = trans * m_transforms.top();
        }
        void set(const Mat4& trans)
        {
            m_transforms.top() = trans;
        }

    private:
        std::stack<Mat4> m_transforms;
};

/// Class managing the attributes stack.
class AttributesStack
{
    public:
        AttributesStack()
            : m_stack()
        {
            m_stack.push(new Attributes());
        }

        /// Create an additional ref of top attribute on the stack top
        void push()
        {
            assert(!m_stack.empty());
            m_stack.push(m_stack.top());
        }
        /// Pop off the top attribute from the stack
        void pop()
        {
            if(m_stack.empty())
            {
                assert(0 && "Attribute stack empty - can't pop!");
                return;
            }
            m_stack.pop();
        }

        /// Get a writable pointer to the top attribute on the stack
        ///
        /// If the attributes are already referenced elsewhere (eg, by a piece
        /// of geometry which has already passed into the pipeline), then the
        /// top of the stack is cloned so that it can be written to without
        /// disturbing the externally held attribute state.
        const AttributesPtr& attrsWrite()
        {
            assert(!m_stack.empty());
            if(m_stack.top()->useCount() == 1)
                return m_stack.top();
            else
            {
                // Clone the top of the stack if the attributes are held
                // elsewhere.
                AttributesPtr oldTop = m_stack.top();
                m_stack.pop();
                m_stack.push(new Attributes(*oldTop));
                return m_stack.top();
            }
        }
        /// Get a read-only pointer to the top attribute
        ConstAttributesPtr attrsRead() const
        {
            assert(!m_stack.empty());
            ConstAttributesPtr p(m_stack.top());
            return p;
        }

    private:
        std::stack<AttributesPtr> m_stack;
};

//------------------------------------------------------------------------------
struct AllOptions;
typedef boost::intrusive_ptr<AllOptions> AllOptionsPtr;

/// A container for all option-like parts of the renderman state
struct AllOptions : public RefCounted
{
    OptionsPtr opts;
    CameraInfo camInfo;
    DisplayList displays;

    AllOptions() : opts(new Options()) {}

    AllOptionsPtr clone()
    {
        AllOptionsPtr o = new AllOptions(*this);
        o->opts = new Options(*opts);
        return o;
    }
};

typedef boost::intrusive_ptr<AllOptions> AllOptionsPtr;


//------------------------------------------------------------------------------
/// The renderer API.
class RenderApi : public Ri::Renderer
{
    public:
        RenderApi(ApiServices& services);

        virtual RtVoid ArchiveRecord(RtConstToken type, const char* string)
        { }

        // Code generator for autogenerated method declarations
        /*[[[cog
        from codegenutils import *

        riXml = parseXml(riXmlPath)

        for proc in riXml.findall('Procedures/Procedure'):
            if proc.findall('Rib'):
                decl = 'virtual %s;' % (riCxxMethodDecl(proc),)
                cog.outl(wrapDecl(decl, 72, wrapIndent=20))
        ]]]*/
        virtual RtVoid Declare(RtConstString name, RtConstString declaration);
        virtual RtVoid FrameBegin(RtInt number);
        virtual RtVoid FrameEnd();
        virtual RtVoid WorldBegin();
        virtual RtVoid WorldEnd();
        virtual RtVoid IfBegin(RtConstString condition);
        virtual RtVoid ElseIf(RtConstString condition);
        virtual RtVoid Else();
        virtual RtVoid IfEnd();
        virtual RtVoid Format(RtInt xresolution, RtInt yresolution,
                            RtFloat pixelaspectratio);
        virtual RtVoid FrameAspectRatio(RtFloat frameratio);
        virtual RtVoid ScreenWindow(RtFloat left, RtFloat right, RtFloat bottom,
                            RtFloat top);
        virtual RtVoid CropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin,
                            RtFloat ymax);
        virtual RtVoid Projection(RtConstToken name, const ParamList& pList);
        virtual RtVoid Clipping(RtFloat cnear, RtFloat cfar);
        virtual RtVoid ClippingPlane(RtFloat x, RtFloat y, RtFloat z,
                            RtFloat nx, RtFloat ny, RtFloat nz);
        virtual RtVoid DepthOfField(RtFloat fstop, RtFloat focallength,
                            RtFloat focaldistance);
        virtual RtVoid Shutter(RtFloat opentime, RtFloat closetime);
        virtual RtVoid PixelVariance(RtFloat variance);
        virtual RtVoid PixelSamples(RtFloat xsamples, RtFloat ysamples);
        virtual RtVoid PixelFilter(RtFilterFunc function, RtFloat xwidth,
                            RtFloat ywidth);
        virtual RtVoid Exposure(RtFloat gain, RtFloat gamma);
        virtual RtVoid Imager(RtConstToken name, const ParamList& pList);
        virtual RtVoid Quantize(RtConstToken type, RtInt one, RtInt min,
                            RtInt max, RtFloat ditheramplitude);
        virtual RtVoid Display(RtConstToken name, RtConstToken type,
                            RtConstToken mode, const ParamList& pList);
        virtual RtVoid Hider(RtConstToken name, const ParamList& pList);
        virtual RtVoid ColorSamples(const FloatArray& nRGB,
                            const FloatArray& RGBn);
        virtual RtVoid RelativeDetail(RtFloat relativedetail);
        virtual RtVoid Option(RtConstToken name, const ParamList& pList);
        virtual RtVoid AttributeBegin();
        virtual RtVoid AttributeEnd();
        virtual RtVoid Color(RtConstColor Cq);
        virtual RtVoid Opacity(RtConstColor Os);
        virtual RtVoid TextureCoordinates(RtFloat s1, RtFloat t1, RtFloat s2,
                            RtFloat t2, RtFloat s3, RtFloat t3, RtFloat s4,
                            RtFloat t4);
        virtual RtVoid LightSource(RtConstToken shadername, RtConstToken name,
                            const ParamList& pList);
        virtual RtVoid AreaLightSource(RtConstToken shadername,
                            RtConstToken name, const ParamList& pList);
        virtual RtVoid Illuminate(RtConstToken name, RtBoolean onoff);
        virtual RtVoid Surface(RtConstToken name, const ParamList& pList);
        virtual RtVoid Displacement(RtConstToken name, const ParamList& pList);
        virtual RtVoid Atmosphere(RtConstToken name, const ParamList& pList);
        virtual RtVoid Interior(RtConstToken name, const ParamList& pList);
        virtual RtVoid Exterior(RtConstToken name, const ParamList& pList);
        virtual RtVoid ShaderLayer(RtConstToken type, RtConstToken name,
                            RtConstToken layername, const ParamList& pList);
        virtual RtVoid ConnectShaderLayers(RtConstToken type,
                            RtConstToken layer1, RtConstToken variable1,
                            RtConstToken layer2, RtConstToken variable2);
        virtual RtVoid ShadingRate(RtFloat size);
        virtual RtVoid ShadingInterpolation(RtConstToken type);
        virtual RtVoid Matte(RtBoolean onoff);
        virtual RtVoid Bound(RtConstBound bound);
        virtual RtVoid Detail(RtConstBound bound);
        virtual RtVoid DetailRange(RtFloat offlow, RtFloat onlow,
                            RtFloat onhigh, RtFloat offhigh);
        virtual RtVoid GeometricApproximation(RtConstToken type,
                            RtFloat value);
        virtual RtVoid Orientation(RtConstToken orientation);
        virtual RtVoid ReverseOrientation();
        virtual RtVoid Sides(RtInt nsides);
        virtual RtVoid Identity();
        virtual RtVoid Transform(RtConstMatrix transform);
        virtual RtVoid ConcatTransform(RtConstMatrix transform);
        virtual RtVoid Perspective(RtFloat fov);
        virtual RtVoid Translate(RtFloat dx, RtFloat dy, RtFloat dz);
        virtual RtVoid Rotate(RtFloat angle, RtFloat dx, RtFloat dy,
                            RtFloat dz);
        virtual RtVoid Scale(RtFloat sx, RtFloat sy, RtFloat sz);
        virtual RtVoid Skew(RtFloat angle, RtFloat dx1, RtFloat dy1,
                            RtFloat dz1, RtFloat dx2, RtFloat dy2,
                            RtFloat dz2);
        virtual RtVoid CoordinateSystem(RtConstToken space);
        virtual RtVoid CoordSysTransform(RtConstToken space);
        virtual RtVoid TransformBegin();
        virtual RtVoid TransformEnd();
        virtual RtVoid Resource(RtConstToken handle, RtConstToken type,
                            const ParamList& pList);
        virtual RtVoid ResourceBegin();
        virtual RtVoid ResourceEnd();
        virtual RtVoid Attribute(RtConstToken name, const ParamList& pList);
        virtual RtVoid Polygon(const ParamList& pList);
        virtual RtVoid GeneralPolygon(const IntArray& nverts,
                            const ParamList& pList);
        virtual RtVoid PointsPolygons(const IntArray& nverts,
                            const IntArray& verts, const ParamList& pList);
        virtual RtVoid PointsGeneralPolygons(const IntArray& nloops,
                            const IntArray& nverts, const IntArray& verts,
                            const ParamList& pList);
        virtual RtVoid Basis(RtConstBasis ubasis, RtInt ustep,
                            RtConstBasis vbasis, RtInt vstep);
        virtual RtVoid Patch(RtConstToken type, const ParamList& pList);
        virtual RtVoid PatchMesh(RtConstToken type, RtInt nu,
                            RtConstToken uwrap, RtInt nv, RtConstToken vwrap,
                            const ParamList& pList);
        virtual RtVoid NuPatch(RtInt nu, RtInt uorder, const FloatArray& uknot,
                            RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder,
                            const FloatArray& vknot, RtFloat vmin, RtFloat vmax,
                            const ParamList& pList);
        virtual RtVoid TrimCurve(const IntArray& ncurves, const IntArray& order,
                            const FloatArray& knot, const FloatArray& min,
                            const FloatArray& max, const IntArray& n,
                            const FloatArray& u, const FloatArray& v,
                            const FloatArray& w);
        virtual RtVoid SubdivisionMesh(RtConstToken scheme,
                            const IntArray& nvertices, const IntArray& vertices,
                            const TokenArray& tags, const IntArray& nargs,
                            const IntArray& intargs,
                            const FloatArray& floatargs,
                            const ParamList& pList);
        virtual RtVoid Sphere(RtFloat radius, RtFloat zmin, RtFloat zmax,
                            RtFloat thetamax, const ParamList& pList);
        virtual RtVoid Cone(RtFloat height, RtFloat radius, RtFloat thetamax,
                            const ParamList& pList);
        virtual RtVoid Cylinder(RtFloat radius, RtFloat zmin, RtFloat zmax,
                            RtFloat thetamax, const ParamList& pList);
        virtual RtVoid Hyperboloid(RtConstPoint point1, RtConstPoint point2,
                            RtFloat thetamax, const ParamList& pList);
        virtual RtVoid Paraboloid(RtFloat rmax, RtFloat zmin, RtFloat zmax,
                            RtFloat thetamax, const ParamList& pList);
        virtual RtVoid Disk(RtFloat height, RtFloat radius, RtFloat thetamax,
                            const ParamList& pList);
        virtual RtVoid Torus(RtFloat majorrad, RtFloat minorrad, RtFloat phimin,
                            RtFloat phimax, RtFloat thetamax,
                            const ParamList& pList);
        virtual RtVoid Points(const ParamList& pList);
        virtual RtVoid Curves(RtConstToken type, const IntArray& nvertices,
                            RtConstToken wrap, const ParamList& pList);
        virtual RtVoid Blobby(RtInt nleaf, const IntArray& code,
                            const FloatArray& floats, const TokenArray& strings,
                            const ParamList& pList);
        virtual RtVoid Procedural(RtPointer data, RtConstBound bound,
                            RtProcSubdivFunc refineproc,
                            RtProcFreeFunc freeproc);
        virtual RtVoid Geometry(RtConstToken type, const ParamList& pList);
        virtual RtVoid SolidBegin(RtConstToken type);
        virtual RtVoid SolidEnd();
        virtual RtVoid ObjectBegin(RtConstToken name);
        virtual RtVoid ObjectEnd();
        virtual RtVoid ObjectInstance(RtConstToken name);
        virtual RtVoid MotionBegin(const FloatArray& times);
        virtual RtVoid MotionEnd();
        virtual RtVoid MakeTexture(RtConstString imagefile,
                            RtConstString texturefile, RtConstToken swrap,
                            RtConstToken twrap, RtFilterFunc filterfunc,
                            RtFloat swidth, RtFloat twidth,
                            const ParamList& pList);
        virtual RtVoid MakeLatLongEnvironment(RtConstString imagefile,
                            RtConstString reflfile, RtFilterFunc filterfunc,
                            RtFloat swidth, RtFloat twidth,
                            const ParamList& pList);
        virtual RtVoid MakeCubeFaceEnvironment(RtConstString px,
                            RtConstString nx, RtConstString py,
                            RtConstString ny, RtConstString pz,
                            RtConstString nz, RtConstString reflfile,
                            RtFloat fov, RtFilterFunc filterfunc,
                            RtFloat swidth, RtFloat twidth,
                            const ParamList& pList);
        virtual RtVoid MakeShadow(RtConstString picfile,
                            RtConstString shadowfile, const ParamList& pList);
        virtual RtVoid MakeOcclusion(const StringArray& picfiles,
                            RtConstString shadowfile, const ParamList& pList);
        virtual RtVoid ErrorHandler(RtErrorFunc handler);
        virtual RtVoid ReadArchive(RtConstToken name,
                            RtArchiveCallback callback,
                            const ParamList& pList);
        virtual RtVoid ArchiveBegin(RtConstToken name, const ParamList& pList);
        virtual RtVoid ArchiveEnd();
        ///[[[end]]]

    private:
        Ri::ErrorHandler& ehandler() { return m_services.errorHandler(); }

        const AttributesPtr& attrsWrite() { return m_attrStack.attrsWrite(); }
        ConstAttributesPtr attrsRead() const { return m_attrStack.attrsRead(); }

        ApiServices& m_services;
        AllOptionsPtr m_opts;
        AllOptionsPtr m_savedOpts;
        AttributesStack m_attrStack;
        TransformStack m_transStack;
        boost::shared_ptr< Aqsis::Renderer> m_renderer;
};


RenderApi::RenderApi(ApiServices& services)
    : m_services(services),
    m_opts(new AllOptions()),
    m_savedOpts(),
    m_attrStack(),
    m_transStack(),
    m_renderer()
{ }

//------------------------------------------------------------------------------
/*
For reference, the method stubs below were originally generated with the
following cog code generator:

from Cheetah.Template import Template

methodTmpl = '''
$wrapDecl($riCxxMethodDecl($proc, className='RenderApi'), 80)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "$procName not implemented"; // Todo
}
'''

for proc in riXml.findall('Procedures/'+'*'):
    if proc.tag == 'Procedure':
        if proc.findall('Rib'):
            procName = proc.findtext('Name')
            cog.out(str(Template(methodTmpl, searchList=locals())))
    elif proc.tag == 'Section' or proc.tag == 'SubSection':
        cog.outl('')
        cog.outl('//------------------------------------------------------------')
        cog.outl('// ' + proc.findtext('.'))

*/
//------------------------------------------------------------------------------

RtVoid RenderApi::Declare(RtConstString name, RtConstString declaration)
{
    m_services.declare(name, declaration);
}

//------------------------------------------------------------
// Graphics State

RtVoid RenderApi::FrameBegin(RtInt number)
{
    // Clone options
    m_savedOpts = m_opts;
    m_opts = m_opts->clone();
    // Save transformation
    m_transStack.push();
    m_attrStack.push();
}

RtVoid RenderApi::FrameEnd()
{
    assert(m_savedOpts);
    m_opts = m_savedOpts;
    m_transStack.pop();
    m_attrStack.pop();
}

RtVoid RenderApi::WorldBegin()
{
    Mat4 camToScreen = m_opts->camInfo.camToScreenMatrix(*m_opts->opts);
    m_renderer.reset(new Aqsis::Renderer(m_opts->opts, camToScreen,
                                         m_opts->displays));
    m_attrStack.push();
    m_transStack.push();
}

RtVoid RenderApi::WorldEnd()
{
    m_renderer->render();
    m_renderer.reset();
    m_transStack.pop();
    m_attrStack.pop();
}

//------------------------------------------------------------
// Conditional RIB

RtVoid RenderApi::IfBegin(RtConstString condition)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "IfBegin not implemented"; // Todo
}

RtVoid RenderApi::ElseIf(RtConstString condition)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ElseIf not implemented"; // Todo
}

RtVoid RenderApi::Else()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Else not implemented"; // Todo
}

RtVoid RenderApi::IfEnd()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "IfEnd not implemented"; // Todo
}

//------------------------------------------------------------
// Options

RtVoid RenderApi::Format(RtInt xresolution, RtInt yresolution,
                         RtFloat pixelaspectratio)
{
    m_opts->opts->resolution = V2i(xresolution, yresolution);
    m_opts->camInfo.setFrameAspect(pixelaspectratio*xresolution/yresolution, false);
}

RtVoid RenderApi::FrameAspectRatio(RtFloat frameratio)
{
    m_opts->camInfo.setFrameAspect(frameratio, true);
}

RtVoid RenderApi::ScreenWindow(RtFloat left, RtFloat right, RtFloat bottom,
                               RtFloat top)
{
    m_opts->camInfo.setScreenWindow(left, right, bottom, top);
}

RtVoid RenderApi::CropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin,
                             RtFloat ymax)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "CropWindow not implemented"; // Todo
}

RtVoid RenderApi::Projection(RtConstToken name, const ParamList& pList)
{
    if(!name)
    {
        m_opts->camInfo.setType(CameraInfo::UserDefined);
    }
    else if(strcmp(name, "perspective") == 0)
    {
        m_opts->camInfo.setType(CameraInfo::Perspective);
        FloatArray fov = pList.findFloatData(Ri::TypeSpec::Float, "fov");
        if(fov)
            m_opts->camInfo.setFov(fov[0]);
    }
    else if(strcmp(name, "orthographic") == 0)
    {
        m_opts->camInfo.setType(CameraInfo::Orthographic);
    }
    else
    {
        AQSIS_THROW_XQERROR(XqValidation, EqE_BadToken,
            "Unknown projection type " << name);
    }
}

RtVoid RenderApi::Clipping(RtFloat cnear, RtFloat cfar)
{
    m_opts->opts->clipNear = cnear;
    m_opts->opts->clipFar = cfar;
}

RtVoid RenderApi::ClippingPlane(RtFloat x, RtFloat y, RtFloat z, RtFloat nx,
                                RtFloat ny, RtFloat nz)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ClippingPlane not implemented"; // Todo
}

RtVoid RenderApi::DepthOfField(RtFloat fstop, RtFloat focallength,
                               RtFloat focaldistance)
{
    m_opts->opts->fstop = fstop;
    m_opts->opts->focalLength = focallength;
    m_opts->opts->focalDistance = focaldistance;
}

RtVoid RenderApi::Shutter(RtFloat opentime, RtFloat closetime)
{
    m_opts->opts->shutterMin = opentime;
    m_opts->opts->shutterMax = closetime;
}

RtVoid RenderApi::PixelVariance(RtFloat variance)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "PixelVariance not implemented"; // Todo
}

RtVoid RenderApi::PixelSamples(RtFloat xsamples, RtFloat ysamples)
{
    m_opts->opts->superSamp = Imath::V2i(xsamples, ysamples);
}

RtVoid RenderApi::PixelFilter(RtFilterFunc function, RtFloat xwidth,
                              RtFloat ywidth)
{
    if((RtFilterFunc)1 == function)
        m_opts->opts->pixelFilter = makeBoxFilter(Vec2(xwidth,ywidth));
    else if((RtFilterFunc)2 == function)
        m_opts->opts->pixelFilter = makeGaussianFilter(Vec2(xwidth,ywidth));
    else if((RtFilterFunc)3 == function)
        m_opts->opts->pixelFilter = makeSincFilter(Vec2(xwidth,ywidth));
    else if((RtFilterFunc)4 == function)
        m_opts->opts->pixelFilter = makeDiscFilter(Vec2(xwidth,ywidth));
    else
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Unimplemented pixel filter function";
}

RtVoid RenderApi::Exposure(RtFloat gain, RtFloat gamma)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Exposure not implemented"; // Todo
}

RtVoid RenderApi::Imager(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Imager not implemented"; // Todo
}

RtVoid RenderApi::Quantize(RtConstToken type, RtInt one, RtInt min, RtInt max,
                           RtFloat ditheramplitude)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Quantize not implemented"; // Todo
}

RtVoid RenderApi::Display(RtConstToken name, RtConstToken type,
                          RtConstToken mode, const ParamList& pList)
{
    // Determine which output variable name to use
    VarSpec outVar;
    if(strcmp(mode, "rgb") == 0)
        outVar = Stdvar::Ci;
    else if(strcmp(mode, "rgba") == 0)
    {
        // TODO
        outVar = Stdvar::Ci;
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "rgba output unimplemented, using rgb.";
    }
    else if(strcmp(mode, "z") == 0)
        outVar = Stdvar::z;
    else
    {
        const char* nameBegin = 0;
        const char* nameEnd = 0;
        outVar = typeSpecToVarSpec(m_services.getDeclaration(mode, &nameBegin,
                                                             &nameEnd));
        outVar.name.assign(std::string(nameBegin, nameEnd));
    }

    // Display names starting with '+' indicate appending a display to the
    // current list.  If '+' is not present, clear the display list instead.
    if(name[0] == '+')
        ++name;
    else
        m_opts->displays.clear();

    // Create the display object
    if(!m_opts->displays.addDisplay(name, type, outVar, pList))
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Could not open display type \"" << type << "\"";
        return;
    }
}

RtVoid RenderApi::Hider(RtConstToken name, const ParamList& pList)
{
    if(strcmp(name, "hidden") != 0)
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Hider " << name << " not implemented";
        return;
    }
    ParamListUsage params(pList);
    if(IntArray subpixel = params.findIntData(Ri::TypeSpec::Int, "subpixel"))
        m_opts->opts->doFilter = subpixel[0];
    if(params.hasUnusedParams())
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Unrecognized parameters to Hider: "
            << params.unusedParams();
    }
}

RtVoid RenderApi::ColorSamples(const FloatArray& nRGB, const FloatArray& RGBn)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ColorSamples not implemented"; // Todo
}

RtVoid RenderApi::RelativeDetail(RtFloat relativedetail)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "RelativeDetail not implemented"; // Todo
}

RtVoid RenderApi::Option(RtConstToken name, const ParamList& pList)
{
    ParamListUsage params(pList);
    if(strcmp(name, "limits") == 0)
    {
        if(IntArray bs = params.findIntData(
                        Ri::TypeSpec(Ri::TypeSpec::Int, 2), "bucketsize"))
            m_opts->opts->bucketSize = Vec2(bs[0], bs[1]);
        if(IntArray gs = params.findIntData(Ri::TypeSpec::Int, "gridsize"))
            m_opts->opts->gridSize = ifloor(sqrt(gs[0]));
        if(IntArray es = params.findIntData(Ri::TypeSpec::Int, "eyesplits"))
            m_opts->opts->eyeSplits = es[0];
        if(IntArray t = params.findIntData(Ri::TypeSpec::Int, "threads"))
            m_opts->opts->nthreads = t[0];
    }
    if(strcmp(name, "statistics") == 0)
    {
        if(IntArray i = params.findIntData(Ri::TypeSpec::Int, "endofframe"))
            m_opts->opts->statsVerbosity = i[0];
    }
    if(params.hasUnusedParams())
    {
        // Complain if we failed to handle at least one option.
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Unrecognized parameters to Option \"" << name << "\": "
            << params.unusedParams();
    }
}

//------------------------------------------------------------
// Attributes

RtVoid RenderApi::AttributeBegin()
{
    m_attrStack.push();
    m_transStack.push();
}

RtVoid RenderApi::AttributeEnd()
{
    m_attrStack.pop();
    m_transStack.pop();
}

RtVoid RenderApi::Color(RtConstColor Cq)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Color not implemented"; // Todo
}

RtVoid RenderApi::Opacity(RtConstColor Os)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Opacity not implemented"; // Todo
}

RtVoid RenderApi::TextureCoordinates(RtFloat s1, RtFloat t1, RtFloat s2,
                                     RtFloat t2, RtFloat s3, RtFloat t3,
                                     RtFloat s4, RtFloat t4)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "TextureCoordinates not implemented"; // Todo
}

RtVoid RenderApi::LightSource(RtConstToken shadername, RtConstToken name,
                              const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "LightSource not implemented"; // Todo
}

RtVoid RenderApi::AreaLightSource(RtConstToken shadername, RtConstToken name,
                                  const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "AreaLightSource not implemented"; // Todo
}

RtVoid RenderApi::Illuminate(RtConstToken name, RtBoolean onoff)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Illuminate not implemented"; // Todo
}

RtVoid RenderApi::Surface(RtConstToken name, const ParamList& pList)
{
    if(strcmp(name, "null") == 0)
    {
        attrsWrite()->surfaceShader.reset();
        return;
    }
    attrsWrite()->surfaceShader = createShader(name);
    if(!attrsRead()->surfaceShader)
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "unimplemented shader \"" << name <<"\"";
    }
}

RtVoid RenderApi::Displacement(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Displacement not implemented"; // Todo
}

RtVoid RenderApi::Atmosphere(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Atmosphere not implemented"; // Todo
}

RtVoid RenderApi::Interior(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Interior not implemented"; // Todo
}

RtVoid RenderApi::Exterior(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Exterior not implemented"; // Todo
}

RtVoid RenderApi::ShaderLayer(RtConstToken type, RtConstToken name,
                              RtConstToken layername, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ShaderLayer not implemented"; // Todo
}

RtVoid RenderApi::ConnectShaderLayers(RtConstToken type, RtConstToken layer1,
                                      RtConstToken variable1,
                                      RtConstToken layer2,
                                      RtConstToken variable2)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ConnectShaderLayers not implemented"; // Todo
}

RtVoid RenderApi::ShadingRate(RtFloat size)
{
    attrsWrite()->shadingRate = size;
}

RtVoid RenderApi::ShadingInterpolation(RtConstToken type)
{
    if(strcmp(type, "constant") == 0)
        attrsWrite()->smoothShading = false;
    else
    {
        if(strcmp(type, "smooth") != 0)
        {
            AQSIS_LOG_WARNING(ehandler(), EqE_BadToken)
                << "unrecognized shading interpolation type \""
                << type << "\", using smooth";
        }
        attrsWrite()->smoothShading = true;
    }
}

RtVoid RenderApi::Matte(RtBoolean onoff)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Matte not implemented"; // Todo
}

RtVoid RenderApi::Bound(RtConstBound bound)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Bound not implemented"; // Todo
}

RtVoid RenderApi::Detail(RtConstBound bound)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Detail not implemented"; // Todo
}

RtVoid RenderApi::DetailRange(RtFloat offlow, RtFloat onlow, RtFloat onhigh,
                              RtFloat offhigh)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "DetailRange not implemented"; // Todo
}

RtVoid RenderApi::GeometricApproximation(RtConstToken type, RtFloat value)
{
    if(strcmp(type, "focusfactor") == 0)
    {
        attrsWrite()->focusFactor = value;
    }
    else
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Unknown GeometricApproximation type \"" << type << "\"";
    }
}

RtVoid RenderApi::Orientation(RtConstToken orientation)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Orientation not implemented"; // Todo
}

RtVoid RenderApi::ReverseOrientation()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ReverseOrientation not implemented"; // Todo
}

RtVoid RenderApi::Sides(RtInt nsides)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Sides not implemented"; // Todo
}

//------------------------------------------------------------
// Transformations

RtVoid RenderApi::Identity()
{
    m_transStack.set(Mat4());
}

RtVoid RenderApi::Transform(RtConstMatrix transform)
{
    m_transStack.set(Mat4(transform));
}

RtVoid RenderApi::ConcatTransform(RtConstMatrix transform)
{
    m_transStack.concat(Mat4(transform));
}

RtVoid RenderApi::Perspective(RtFloat fov)
{
    float s = std::tan(deg2rad(fov/2));
    // Old core notes: "This matches PRMan 3.9 in testing, but not BMRT 2.6's
    // rgl and rendrib."
    Mat4 p(1, 0,  0, 0,
           0, 1,  0, 0,
           0, 0,  s, s,
           0, 0, -s, 0);
    m_transStack.concat(p);
}

RtVoid RenderApi::Translate(RtFloat dx, RtFloat dy, RtFloat dz)
{
    m_transStack.concat(Mat4().setTranslation(Vec3(dx,dy,dz)));
}

RtVoid RenderApi::Rotate(RtFloat angle, RtFloat dx, RtFloat dy, RtFloat dz)
{
    m_transStack.concat(Mat4().setAxisAngle(Vec3(dx,dy,dz), deg2rad(angle)));
}

RtVoid RenderApi::Scale(RtFloat sx, RtFloat sy, RtFloat sz)
{
    m_transStack.concat(Mat4().setScale(Vec3(sx,sy,sz)));
}

RtVoid RenderApi::Skew(RtFloat angle, RtFloat dx1, RtFloat dy1, RtFloat dz1,
                       RtFloat dx2, RtFloat dy2, RtFloat dz2)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Skew not implemented"; // Todo
}

RtVoid RenderApi::CoordinateSystem(RtConstToken space)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "CoordinateSystem not implemented"; // Todo
}

RtVoid RenderApi::CoordSysTransform(RtConstToken space)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "CoordSysTransform not implemented"; // Todo
}

RtVoid RenderApi::TransformBegin()
{
    m_transStack.push();
}

RtVoid RenderApi::TransformEnd()
{
    m_transStack.pop();
}

//------------------------------------------------------------
// Resources

RtVoid RenderApi::Resource(RtConstToken handle, RtConstToken type,
                           const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Resource not implemented"; // Todo
}

RtVoid RenderApi::ResourceBegin()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ResourceBegin not implemented"; // Todo
}

RtVoid RenderApi::ResourceEnd()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ResourceEnd not implemented"; // Todo
}

//------------------------------------------------------------
// Implementation-specific Attributes

RtVoid RenderApi::Attribute(RtConstToken name, const ParamList& pList)
{
    if(strcmp(name, "displacementbound") == 0)
    {
        if(FloatArray s = pList.findFloatData(Ri::TypeSpec::Float, "sphere"))
            attrsWrite()->displacementBound = s[0];
        // TODO Arbitrary coordinate systems
    }
}

//------------------------------------------------------------
// Geometric Primitives

//------------------------------------------------------------
// Polygons

RtVoid RenderApi::Polygon(const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Polygon not implemented"; // Todo
}

RtVoid RenderApi::GeneralPolygon(const IntArray& nverts,
                                 const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "GeneralPolygon not implemented"; // Todo
}

RtVoid RenderApi::PointsPolygons(const IntArray& nverts, const IntArray& verts,
                                 const ParamList& pList)
{
    ConstDataView<Vec3> P(pList.findFloatData(Ri::TypeSpec(Ri::TypeSpec::Vertex, Ri::TypeSpec::Point), "P").begin());
    ConstDataView<Vec3> N(pList.findFloatData(Ri::TypeSpec(Ri::TypeSpec::Varying, Ri::TypeSpec::Normal), "N").begin());
    ConstDataView<Col3> Cs(pList.findFloatData(Ri::TypeSpec(Ri::TypeSpec::Varying, Ri::TypeSpec::Color), "Cs").begin());
    float P_stor[12];
    float N_stor[12];
    float Cs_stor[12];
    for(size_t face = 0; face < nverts.size(); ++face)
    {
        if(nverts[face] != 4)
        {
            AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
                << "Ignoring non-quad face in PointsPolygons";
            continue;
        }
        PrimvarStorageBuilder builder;
#       define COPYVAR(dest, src) \
            dest[0] = src[verts[4*face+0]]; dest[1] = src[verts[4*face+1]]; \
            dest[2] = src[verts[4*face+3]]; dest[3] = src[verts[4*face+2]];
        {
            DataView<Vec3> view(P_stor);
            COPYVAR(view, P);
            builder.add(Primvar::P, P_stor, 12);
        }
        if(N)
        {
            DataView<Vec3> view(N_stor);
            COPYVAR(view, N);
            builder.add(Primvar::N, N_stor, 12);
        }
        if(Cs)
        {
            DataView<Col3> view(Cs_stor);
            COPYVAR(view, Cs);
            builder.add(Primvar::Cs, Cs_stor, 12);
        }
        else
        {
            float Cs_in[] = {1, 1, 1};
            DataView<Col3> Cs(Cs_in, 0);
            DataView<Col3> view(Cs_stor);
            COPYVAR(view, Cs);
            builder.add(Primvar::Cs, Cs_stor, 12);
        }
        IclassStorage storReq(1,4,4,4,4);
        GeometryPtr patch(new Aqsis::Patch(builder.build(storReq)));
        patch->transform(m_transStack.top());
        m_renderer->add(patch, attrsRead());
    }
}

RtVoid RenderApi::PointsGeneralPolygons(const IntArray& nloops,
                                        const IntArray& nverts,
                                        const IntArray& verts,
                                        const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "PointsGeneralPolygons not implemented"; // Todo
}

//------------------------------------------------------------
// Patches

RtVoid RenderApi::Basis(RtConstBasis ubasis, RtInt ustep, RtConstBasis vbasis,
                        RtInt vstep)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Basis not implemented"; // Todo
}

RtVoid RenderApi::Patch(RtConstToken type, const ParamList& pList)
{
    if(strcmp(type, "bilinear") == 0)
    {
        FloatArray P = pList.findFloatData(
                Ri::TypeSpec(Ri::TypeSpec::Vertex, Ri::TypeSpec::Point), "P");
        assert(P.begin());
        PrimvarStorageBuilder builder;
        builder.add(Primvar::P, P.begin(), P.size());
        IclassStorage storReq(1,4,4,4,4);
        GeometryPtr patch(new Aqsis::Patch(builder.build(storReq)));
        patch->transform(m_transStack.top());
        m_renderer->add(patch, attrsRead());
    }
    else if(strcmp(type, "bicubic") == 0)
    {
        AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
            << "Cubic patches not implemented";
    }
    else
    {
        AQSIS_THROW_XQERROR(XqValidation, EqE_BadToken,
            "Unknown patch type \"" << type << "\"");
    }
}

RtVoid RenderApi::PatchMesh(RtConstToken type, RtInt nu, RtConstToken uwrap,
                            RtInt nv, RtConstToken vwrap,
                            const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "PatchMesh not implemented"; // Todo
}

RtVoid RenderApi::NuPatch(RtInt nu, RtInt uorder, const FloatArray& uknot,
                          RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder,
                          const FloatArray& vknot, RtFloat vmin, RtFloat vmax,
                          const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "NuPatch not implemented"; // Todo
}

RtVoid RenderApi::TrimCurve(const IntArray& ncurves, const IntArray& order,
                            const FloatArray& knot, const FloatArray& min,
                            const FloatArray& max, const IntArray& n,
                            const FloatArray& u, const FloatArray& v,
                            const FloatArray& w)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "TrimCurve not implemented"; // Todo
}

//------------------------------------------------------------
// Subdivision Surfaces

RtVoid RenderApi::SubdivisionMesh(RtConstToken scheme,
                                  const IntArray& nvertices,
                                  const IntArray& vertices,
                                  const TokenArray& tags, const IntArray& nargs,
                                  const IntArray& intargs,
                                  const FloatArray& floatargs,
                                  const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "SubdivisionMesh not implemented"; // Todo
}

//------------------------------------------------------------
// Quadrics

RtVoid RenderApi::Sphere(RtFloat radius, RtFloat zmin, RtFloat zmax,
                         RtFloat thetamax, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Sphere not implemented"; // Todo
}

RtVoid RenderApi::Cone(RtFloat height, RtFloat radius, RtFloat thetamax,
                       const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Cone not implemented"; // Todo
}

RtVoid RenderApi::Cylinder(RtFloat radius, RtFloat zmin, RtFloat zmax,
                           RtFloat thetamax, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Cylinder not implemented"; // Todo
}

RtVoid RenderApi::Hyperboloid(RtConstPoint point1, RtConstPoint point2,
                              RtFloat thetamax, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Hyperboloid not implemented"; // Todo
}

RtVoid RenderApi::Paraboloid(RtFloat rmax, RtFloat zmin, RtFloat zmax,
                             RtFloat thetamax, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Paraboloid not implemented"; // Todo
}

RtVoid RenderApi::Disk(RtFloat height, RtFloat radius, RtFloat thetamax,
                       const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Disk not implemented"; // Todo
}

RtVoid RenderApi::Torus(RtFloat majorrad, RtFloat minorrad, RtFloat phimin,
                        RtFloat phimax, RtFloat thetamax,
                        const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Torus not implemented"; // Todo
}

//------------------------------------------------------------
// Points and Curve Primitives

RtVoid RenderApi::Points(const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Points not implemented"; // Todo
}

RtVoid RenderApi::Curves(RtConstToken type, const IntArray& nvertices,
                         RtConstToken wrap, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Curves not implemented"; // Todo
}

//------------------------------------------------------------
// Blobby Implicit Surfaces

RtVoid RenderApi::Blobby(RtInt nleaf, const IntArray& code,
                         const FloatArray& floats, const TokenArray& strings,
                         const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Blobby not implemented"; // Todo
}

//------------------------------------------------------------
// Procedural Primitives

RtVoid RenderApi::Procedural(RtPointer data, RtConstBound bound,
                             RtProcSubdivFunc refineproc,
                             RtProcFreeFunc freeproc)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Procedural not implemented"; // Todo
}

//------------------------------------------------------------
// Implementation-specific Geometric Primitives

RtVoid RenderApi::Geometry(RtConstToken type, const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "Geometry not implemented"; // Todo
}

//------------------------------------------------------------
// Soids and Spatial Set Operations

RtVoid RenderApi::SolidBegin(RtConstToken type)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "SolidBegin not implemented"; // Todo
}

RtVoid RenderApi::SolidEnd()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "SolidEnd not implemented"; // Todo
}

//------------------------------------------------------------
// Retained Geometry

RtVoid RenderApi::ObjectBegin(RtConstToken name)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ObjectBegin not implemented"; // Todo
}

RtVoid RenderApi::ObjectEnd()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ObjectEnd not implemented"; // Todo
}

RtVoid RenderApi::ObjectInstance(RtConstToken name)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ObjectInstance not implemented"; // Todo
}

//------------------------------------------------------------
// Motion

RtVoid RenderApi::MotionBegin(const FloatArray& times)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MotionBegin not implemented"; // Todo
}

RtVoid RenderApi::MotionEnd()
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MotionEnd not implemented"; // Todo
}

//------------------------------------------------------------
// External Resources

//------------------------------------------------------------
// Texture Map Utilities

RtVoid RenderApi::MakeTexture(RtConstString imagefile,
                              RtConstString texturefile, RtConstToken swrap,
                              RtConstToken twrap, RtFilterFunc filterfunc,
                              RtFloat swidth, RtFloat twidth,
                              const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MakeTexture not implemented"; // Todo
}

RtVoid RenderApi::MakeLatLongEnvironment(RtConstString imagefile,
                                         RtConstString reflfile,
                                         RtFilterFunc filterfunc,
                                         RtFloat swidth, RtFloat twidth,
                                         const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MakeLatLongEnvironment not implemented"; // Todo
}

RtVoid RenderApi::MakeCubeFaceEnvironment(RtConstString px, RtConstString nx,
                                          RtConstString py, RtConstString ny,
                                          RtConstString pz, RtConstString nz,
                                          RtConstString reflfile, RtFloat fov,
                                          RtFilterFunc filterfunc,
                                          RtFloat swidth, RtFloat twidth,
                                          const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MakeCubeFaceEnvironment not implemented"; // Todo
}

RtVoid RenderApi::MakeShadow(RtConstString picfile, RtConstString shadowfile,
                             const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MakeShadow not implemented"; // Todo
}

RtVoid RenderApi::MakeOcclusion(const StringArray& picfiles,
                                RtConstString shadowfile,
                                const ParamList& pList)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "MakeOcclusion not implemented"; // Todo
}

//------------------------------------------------------------
// Errors

RtVoid RenderApi::ErrorHandler(RtErrorFunc handler)
{
    AQSIS_LOG_WARNING(ehandler(), EqE_Unimplement)
        << "ErrorHandler not implemented"; // Todo
}

//------------------------------------------------------------
// Archive Files

RtVoid RenderApi::ReadArchive(RtConstToken name, RtArchiveCallback callback,
                              const ParamList& pList)
{
    // TODO: Search path handling
    std::ifstream archive(name, std::ios::in | std::ios::binary);
    if(!name)
    {
        AQSIS_THROW_XQERROR(XqValidation, EqE_BadFile,
            "Cound not open archive file " << name);
    }
    m_services.parseRib(archive, name);
}

RtVoid RenderApi::ArchiveBegin(RtConstToken name, const ParamList& pList)
{
    AQSIS_LOG_ERROR(ehandler(), EqE_Unimplement)
        << "ArchiveBegin should be handled by a filter";
}

RtVoid RenderApi::ArchiveEnd()
{
    AQSIS_LOG_ERROR(ehandler(), EqE_Unimplement)
        << "ArchiveEnd should be handled by a filter";
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ApiServices::ApiServices()
    : m_api(),
    m_parser(),
    m_filterChain(),
    m_errorHandler()
{
    m_api.reset(new RenderApi(*this));
    m_filterChain.push_back(boost::shared_ptr<Ri::Renderer>(
        createRenderUtilFilter(*this, firstFilter()) ));
    addFilter("validate");
}

} // anon. namespace

//--------------------------------------------------
Ri::RendererServices* createRenderer()
{
    return new ApiServices();
}

} // namespace Aqsis
