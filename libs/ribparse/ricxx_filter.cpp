// Aqsis
// Copyright (C) 1997 - 2007, Paul C. Gregory
//
// Contact: pgregory@aqsis.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

/// \file
/// \brief Implementation of filter utilities & base classes
/// \author Chris Foster [chris42f (at) g mail (d0t) com]

#include <aqsis/riutil/ricxx_filter.h>

#include <aqsis/util/exception.h>

namespace Aqsis {
//------------------------------------------------------------------------------

// These filters are implemented elsewhere, declare the functions here for
// simplicity.
Ri::Renderer* createValidateFilter(Ri::RendererServices& services,
                        Ri::Renderer& out, const Ri::ParamList& pList);
Ri::Renderer* createFrameDropFilter(Ri::RendererServices& services,
                        Ri::Renderer& out, const Ri::ParamList& pList);
Ri::Renderer* createInlineArchiveFilter(Ri::RendererServices& services,
                        Ri::Renderer& out, const Ri::ParamList& pList);


Ri::Renderer* createFilter(const char* name, Ri::RendererServices& services,
                           Ri::Renderer& out, const Ri::ParamList& pList)
{
    if(!strcmp(name, "validate"))
    {
        return createValidateFilter(services, out, pList);
    }
    else if(!strcmp(name, "framedrop"))
    {
        return createFrameDropFilter(services, out, pList);
    }
    else if(!strcmp(name, "inlinearchive"))
    {
        return createInlineArchiveFilter(services, out, pList);
    }
    else
    {
        AQSIS_THROW_XQERROR(XqValidation, EqE_System,
            "could not find filter \"" << name << "\"");
    }
}

//------------------------------------------------------------------------------
// OnOffFilter implementation

OnOffFilter::OnOffFilter(Ri::RendererServices& services, Ri::Renderer& out)
    : Ri::Filter(services, out),
    m_isActive(true)
{ }

RtVoid OnOffFilter::ArchiveRecord(RtConstToken type, const char* string)
{
    if(m_isActive)
        nextFilter().ArchiveRecord(type, string);
}

// Autogenerated method implementations

/*[[[cog
from codegenutils import *
riXml = parseXml(riXmlPath)
from Cheetah.Template import Template

methodTemplate = r'''
$wrapDecl($riCxxMethodDecl($proc, className='OnOffFilter'), 80)
{
    if(m_isActive)
        nextFilter().${procName}(${', '.join($wrapperCallArgList($proc))});
}
'''

for proc in riXml.findall('Procedures/Procedure'):
    if proc.findall('Rib'):
        procName = proc.findtext('Name')
        cog.out(str(Template(methodTemplate, searchList=locals())));

]]]*/

RtVoid OnOffFilter::Declare(RtConstString name, RtConstString declaration)
{
    if(m_isActive)
        nextFilter().Declare(name, declaration);
}

RtVoid OnOffFilter::FrameBegin(RtInt number)
{
    if(m_isActive)
        nextFilter().FrameBegin(number);
}

RtVoid OnOffFilter::FrameEnd()
{
    if(m_isActive)
        nextFilter().FrameEnd();
}

RtVoid OnOffFilter::WorldBegin()
{
    if(m_isActive)
        nextFilter().WorldBegin();
}

RtVoid OnOffFilter::WorldEnd()
{
    if(m_isActive)
        nextFilter().WorldEnd();
}

RtVoid OnOffFilter::IfBegin(RtConstString condition)
{
    if(m_isActive)
        nextFilter().IfBegin(condition);
}

RtVoid OnOffFilter::ElseIf(RtConstString condition)
{
    if(m_isActive)
        nextFilter().ElseIf(condition);
}

RtVoid OnOffFilter::Else()
{
    if(m_isActive)
        nextFilter().Else();
}

RtVoid OnOffFilter::IfEnd()
{
    if(m_isActive)
        nextFilter().IfEnd();
}

RtVoid OnOffFilter::Format(RtInt xresolution, RtInt yresolution,
                           RtFloat pixelaspectratio)
{
    if(m_isActive)
        nextFilter().Format(xresolution, yresolution, pixelaspectratio);
}

RtVoid OnOffFilter::FrameAspectRatio(RtFloat frameratio)
{
    if(m_isActive)
        nextFilter().FrameAspectRatio(frameratio);
}

RtVoid OnOffFilter::ScreenWindow(RtFloat left, RtFloat right, RtFloat bottom,
                                 RtFloat top)
{
    if(m_isActive)
        nextFilter().ScreenWindow(left, right, bottom, top);
}

RtVoid OnOffFilter::CropWindow(RtFloat xmin, RtFloat xmax, RtFloat ymin,
                               RtFloat ymax)
{
    if(m_isActive)
        nextFilter().CropWindow(xmin, xmax, ymin, ymax);
}

RtVoid OnOffFilter::Projection(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Projection(name, pList);
}

RtVoid OnOffFilter::Clipping(RtFloat cnear, RtFloat cfar)
{
    if(m_isActive)
        nextFilter().Clipping(cnear, cfar);
}

RtVoid OnOffFilter::ClippingPlane(RtFloat x, RtFloat y, RtFloat z, RtFloat nx,
                                  RtFloat ny, RtFloat nz)
{
    if(m_isActive)
        nextFilter().ClippingPlane(x, y, z, nx, ny, nz);
}

RtVoid OnOffFilter::DepthOfField(RtFloat fstop, RtFloat focallength,
                                 RtFloat focaldistance)
{
    if(m_isActive)
        nextFilter().DepthOfField(fstop, focallength, focaldistance);
}

RtVoid OnOffFilter::Shutter(RtFloat opentime, RtFloat closetime)
{
    if(m_isActive)
        nextFilter().Shutter(opentime, closetime);
}

RtVoid OnOffFilter::PixelVariance(RtFloat variance)
{
    if(m_isActive)
        nextFilter().PixelVariance(variance);
}

RtVoid OnOffFilter::PixelSamples(RtFloat xsamples, RtFloat ysamples)
{
    if(m_isActive)
        nextFilter().PixelSamples(xsamples, ysamples);
}

RtVoid OnOffFilter::PixelFilter(RtFilterFunc function, RtFloat xwidth,
                                RtFloat ywidth)
{
    if(m_isActive)
        nextFilter().PixelFilter(function, xwidth, ywidth);
}

RtVoid OnOffFilter::Exposure(RtFloat gain, RtFloat gamma)
{
    if(m_isActive)
        nextFilter().Exposure(gain, gamma);
}

RtVoid OnOffFilter::Imager(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Imager(name, pList);
}

RtVoid OnOffFilter::Quantize(RtConstToken type, RtInt one, RtInt min, RtInt max,
                             RtFloat ditheramplitude)
{
    if(m_isActive)
        nextFilter().Quantize(type, one, min, max, ditheramplitude);
}

RtVoid OnOffFilter::Display(RtConstToken name, RtConstToken type,
                            RtConstToken mode, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Display(name, type, mode, pList);
}

RtVoid OnOffFilter::Hider(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Hider(name, pList);
}

RtVoid OnOffFilter::ColorSamples(const FloatArray& nRGB,
                                 const FloatArray& RGBn)
{
    if(m_isActive)
        nextFilter().ColorSamples(nRGB, RGBn);
}

RtVoid OnOffFilter::RelativeDetail(RtFloat relativedetail)
{
    if(m_isActive)
        nextFilter().RelativeDetail(relativedetail);
}

RtVoid OnOffFilter::Option(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Option(name, pList);
}

RtVoid OnOffFilter::AttributeBegin()
{
    if(m_isActive)
        nextFilter().AttributeBegin();
}

RtVoid OnOffFilter::AttributeEnd()
{
    if(m_isActive)
        nextFilter().AttributeEnd();
}

RtVoid OnOffFilter::Color(RtConstColor Cq)
{
    if(m_isActive)
        nextFilter().Color(Cq);
}

RtVoid OnOffFilter::Opacity(RtConstColor Os)
{
    if(m_isActive)
        nextFilter().Opacity(Os);
}

RtVoid OnOffFilter::TextureCoordinates(RtFloat s1, RtFloat t1, RtFloat s2,
                                       RtFloat t2, RtFloat s3, RtFloat t3,
                                       RtFloat s4, RtFloat t4)
{
    if(m_isActive)
        nextFilter().TextureCoordinates(s1, t1, s2, t2, s3, t3, s4, t4);
}

RtVoid OnOffFilter::LightSource(RtConstToken shadername, RtConstToken name,
                                const ParamList& pList)
{
    if(m_isActive)
        nextFilter().LightSource(shadername, name, pList);
}

RtVoid OnOffFilter::AreaLightSource(RtConstToken shadername, RtConstToken name,
                                    const ParamList& pList)
{
    if(m_isActive)
        nextFilter().AreaLightSource(shadername, name, pList);
}

RtVoid OnOffFilter::Illuminate(RtConstToken name, RtBoolean onoff)
{
    if(m_isActive)
        nextFilter().Illuminate(name, onoff);
}

RtVoid OnOffFilter::Surface(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Surface(name, pList);
}

RtVoid OnOffFilter::Displacement(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Displacement(name, pList);
}

RtVoid OnOffFilter::Atmosphere(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Atmosphere(name, pList);
}

RtVoid OnOffFilter::Interior(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Interior(name, pList);
}

RtVoid OnOffFilter::Exterior(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Exterior(name, pList);
}

RtVoid OnOffFilter::ShaderLayer(RtConstToken type, RtConstToken name,
                                RtConstToken layername, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().ShaderLayer(type, name, layername, pList);
}

RtVoid OnOffFilter::ConnectShaderLayers(RtConstToken type, RtConstToken layer1,
                                        RtConstToken variable1,
                                        RtConstToken layer2,
                                        RtConstToken variable2)
{
    if(m_isActive)
        nextFilter().ConnectShaderLayers(type, layer1, variable1, layer2, variable2);
}

RtVoid OnOffFilter::ShadingRate(RtFloat size)
{
    if(m_isActive)
        nextFilter().ShadingRate(size);
}

RtVoid OnOffFilter::ShadingInterpolation(RtConstToken type)
{
    if(m_isActive)
        nextFilter().ShadingInterpolation(type);
}

RtVoid OnOffFilter::Matte(RtBoolean onoff)
{
    if(m_isActive)
        nextFilter().Matte(onoff);
}

RtVoid OnOffFilter::Bound(RtConstBound bound)
{
    if(m_isActive)
        nextFilter().Bound(bound);
}

RtVoid OnOffFilter::Detail(RtConstBound bound)
{
    if(m_isActive)
        nextFilter().Detail(bound);
}

RtVoid OnOffFilter::DetailRange(RtFloat offlow, RtFloat onlow, RtFloat onhigh,
                                RtFloat offhigh)
{
    if(m_isActive)
        nextFilter().DetailRange(offlow, onlow, onhigh, offhigh);
}

RtVoid OnOffFilter::GeometricApproximation(RtConstToken type, RtFloat value)
{
    if(m_isActive)
        nextFilter().GeometricApproximation(type, value);
}

RtVoid OnOffFilter::Orientation(RtConstToken orientation)
{
    if(m_isActive)
        nextFilter().Orientation(orientation);
}

RtVoid OnOffFilter::ReverseOrientation()
{
    if(m_isActive)
        nextFilter().ReverseOrientation();
}

RtVoid OnOffFilter::Sides(RtInt nsides)
{
    if(m_isActive)
        nextFilter().Sides(nsides);
}

RtVoid OnOffFilter::Identity()
{
    if(m_isActive)
        nextFilter().Identity();
}

RtVoid OnOffFilter::Transform(RtConstMatrix transform)
{
    if(m_isActive)
        nextFilter().Transform(transform);
}

RtVoid OnOffFilter::ConcatTransform(RtConstMatrix transform)
{
    if(m_isActive)
        nextFilter().ConcatTransform(transform);
}

RtVoid OnOffFilter::Perspective(RtFloat fov)
{
    if(m_isActive)
        nextFilter().Perspective(fov);
}

RtVoid OnOffFilter::Translate(RtFloat dx, RtFloat dy, RtFloat dz)
{
    if(m_isActive)
        nextFilter().Translate(dx, dy, dz);
}

RtVoid OnOffFilter::Rotate(RtFloat angle, RtFloat dx, RtFloat dy, RtFloat dz)
{
    if(m_isActive)
        nextFilter().Rotate(angle, dx, dy, dz);
}

RtVoid OnOffFilter::Scale(RtFloat sx, RtFloat sy, RtFloat sz)
{
    if(m_isActive)
        nextFilter().Scale(sx, sy, sz);
}

RtVoid OnOffFilter::Skew(RtFloat angle, RtFloat dx1, RtFloat dy1, RtFloat dz1,
                         RtFloat dx2, RtFloat dy2, RtFloat dz2)
{
    if(m_isActive)
        nextFilter().Skew(angle, dx1, dy1, dz1, dx2, dy2, dz2);
}

RtVoid OnOffFilter::CoordinateSystem(RtConstToken space)
{
    if(m_isActive)
        nextFilter().CoordinateSystem(space);
}

RtVoid OnOffFilter::CoordSysTransform(RtConstToken space)
{
    if(m_isActive)
        nextFilter().CoordSysTransform(space);
}

RtVoid OnOffFilter::TransformBegin()
{
    if(m_isActive)
        nextFilter().TransformBegin();
}

RtVoid OnOffFilter::TransformEnd()
{
    if(m_isActive)
        nextFilter().TransformEnd();
}

RtVoid OnOffFilter::Resource(RtConstToken handle, RtConstToken type,
                             const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Resource(handle, type, pList);
}

RtVoid OnOffFilter::ResourceBegin()
{
    if(m_isActive)
        nextFilter().ResourceBegin();
}

RtVoid OnOffFilter::ResourceEnd()
{
    if(m_isActive)
        nextFilter().ResourceEnd();
}

RtVoid OnOffFilter::Attribute(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Attribute(name, pList);
}

RtVoid OnOffFilter::Polygon(const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Polygon(pList);
}

RtVoid OnOffFilter::GeneralPolygon(const IntArray& nverts,
                                   const ParamList& pList)
{
    if(m_isActive)
        nextFilter().GeneralPolygon(nverts, pList);
}

RtVoid OnOffFilter::PointsPolygons(const IntArray& nverts,
                                   const IntArray& verts,
                                   const ParamList& pList)
{
    if(m_isActive)
        nextFilter().PointsPolygons(nverts, verts, pList);
}

RtVoid OnOffFilter::PointsGeneralPolygons(const IntArray& nloops,
                                          const IntArray& nverts,
                                          const IntArray& verts,
                                          const ParamList& pList)
{
    if(m_isActive)
        nextFilter().PointsGeneralPolygons(nloops, nverts, verts, pList);
}

RtVoid OnOffFilter::Basis(RtConstBasis ubasis, RtInt ustep, RtConstBasis vbasis,
                          RtInt vstep)
{
    if(m_isActive)
        nextFilter().Basis(ubasis, ustep, vbasis, vstep);
}

RtVoid OnOffFilter::Patch(RtConstToken type, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Patch(type, pList);
}

RtVoid OnOffFilter::PatchMesh(RtConstToken type, RtInt nu, RtConstToken uwrap,
                              RtInt nv, RtConstToken vwrap,
                              const ParamList& pList)
{
    if(m_isActive)
        nextFilter().PatchMesh(type, nu, uwrap, nv, vwrap, pList);
}

RtVoid OnOffFilter::NuPatch(RtInt nu, RtInt uorder, const FloatArray& uknot,
                            RtFloat umin, RtFloat umax, RtInt nv, RtInt vorder,
                            const FloatArray& vknot, RtFloat vmin, RtFloat vmax,
                            const ParamList& pList)
{
    if(m_isActive)
        nextFilter().NuPatch(nu, uorder, uknot, umin, umax, nv, vorder, vknot, vmin, vmax, pList);
}

RtVoid OnOffFilter::TrimCurve(const IntArray& ncurves, const IntArray& order,
                              const FloatArray& knot, const FloatArray& min,
                              const FloatArray& max, const IntArray& n,
                              const FloatArray& u, const FloatArray& v,
                              const FloatArray& w)
{
    if(m_isActive)
        nextFilter().TrimCurve(ncurves, order, knot, min, max, n, u, v, w);
}

RtVoid OnOffFilter::SubdivisionMesh(RtConstToken scheme,
                                    const IntArray& nvertices,
                                    const IntArray& vertices,
                                    const TokenArray& tags,
                                    const IntArray& nargs,
                                    const IntArray& intargs,
                                    const FloatArray& floatargs,
                                    const ParamList& pList)
{
    if(m_isActive)
        nextFilter().SubdivisionMesh(scheme, nvertices, vertices, tags, nargs, intargs, floatargs, pList);
}

RtVoid OnOffFilter::Sphere(RtFloat radius, RtFloat zmin, RtFloat zmax,
                           RtFloat thetamax, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Sphere(radius, zmin, zmax, thetamax, pList);
}

RtVoid OnOffFilter::Cone(RtFloat height, RtFloat radius, RtFloat thetamax,
                         const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Cone(height, radius, thetamax, pList);
}

RtVoid OnOffFilter::Cylinder(RtFloat radius, RtFloat zmin, RtFloat zmax,
                             RtFloat thetamax, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Cylinder(radius, zmin, zmax, thetamax, pList);
}

RtVoid OnOffFilter::Hyperboloid(RtConstPoint point1, RtConstPoint point2,
                                RtFloat thetamax, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Hyperboloid(point1, point2, thetamax, pList);
}

RtVoid OnOffFilter::Paraboloid(RtFloat rmax, RtFloat zmin, RtFloat zmax,
                               RtFloat thetamax, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Paraboloid(rmax, zmin, zmax, thetamax, pList);
}

RtVoid OnOffFilter::Disk(RtFloat height, RtFloat radius, RtFloat thetamax,
                         const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Disk(height, radius, thetamax, pList);
}

RtVoid OnOffFilter::Torus(RtFloat majorrad, RtFloat minorrad, RtFloat phimin,
                          RtFloat phimax, RtFloat thetamax,
                          const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Torus(majorrad, minorrad, phimin, phimax, thetamax, pList);
}

RtVoid OnOffFilter::Points(const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Points(pList);
}

RtVoid OnOffFilter::Curves(RtConstToken type, const IntArray& nvertices,
                           RtConstToken wrap, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Curves(type, nvertices, wrap, pList);
}

RtVoid OnOffFilter::Blobby(RtInt nleaf, const IntArray& code,
                           const FloatArray& floats, const TokenArray& strings,
                           const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Blobby(nleaf, code, floats, strings, pList);
}

RtVoid OnOffFilter::Procedural(RtPointer data, RtConstBound bound,
                               RtProcSubdivFunc refineproc,
                               RtProcFreeFunc freeproc)
{
    if(m_isActive)
        nextFilter().Procedural(data, bound, refineproc, freeproc);
}

RtVoid OnOffFilter::Geometry(RtConstToken type, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().Geometry(type, pList);
}

RtVoid OnOffFilter::SolidBegin(RtConstToken type)
{
    if(m_isActive)
        nextFilter().SolidBegin(type);
}

RtVoid OnOffFilter::SolidEnd()
{
    if(m_isActive)
        nextFilter().SolidEnd();
}

RtVoid OnOffFilter::ObjectBegin(RtConstToken name)
{
    if(m_isActive)
        nextFilter().ObjectBegin(name);
}

RtVoid OnOffFilter::ObjectEnd()
{
    if(m_isActive)
        nextFilter().ObjectEnd();
}

RtVoid OnOffFilter::ObjectInstance(RtConstToken name)
{
    if(m_isActive)
        nextFilter().ObjectInstance(name);
}

RtVoid OnOffFilter::MotionBegin(const FloatArray& times)
{
    if(m_isActive)
        nextFilter().MotionBegin(times);
}

RtVoid OnOffFilter::MotionEnd()
{
    if(m_isActive)
        nextFilter().MotionEnd();
}

RtVoid OnOffFilter::MakeTexture(RtConstString imagefile,
                                RtConstString texturefile, RtConstToken swrap,
                                RtConstToken twrap, RtFilterFunc filterfunc,
                                RtFloat swidth, RtFloat twidth,
                                const ParamList& pList)
{
    if(m_isActive)
        nextFilter().MakeTexture(imagefile, texturefile, swrap, twrap, filterfunc, swidth, twidth, pList);
}

RtVoid OnOffFilter::MakeLatLongEnvironment(RtConstString imagefile,
                                           RtConstString reflfile,
                                           RtFilterFunc filterfunc,
                                           RtFloat swidth, RtFloat twidth,
                                           const ParamList& pList)
{
    if(m_isActive)
        nextFilter().MakeLatLongEnvironment(imagefile, reflfile, filterfunc, swidth, twidth, pList);
}

RtVoid OnOffFilter::MakeCubeFaceEnvironment(RtConstString px, RtConstString nx,
                                            RtConstString py, RtConstString ny,
                                            RtConstString pz, RtConstString nz,
                                            RtConstString reflfile, RtFloat fov,
                                            RtFilterFunc filterfunc,
                                            RtFloat swidth, RtFloat twidth,
                                            const ParamList& pList)
{
    if(m_isActive)
        nextFilter().MakeCubeFaceEnvironment(px, nx, py, ny, pz, nz, reflfile, fov, filterfunc, swidth, twidth, pList);
}

RtVoid OnOffFilter::MakeShadow(RtConstString picfile, RtConstString shadowfile,
                               const ParamList& pList)
{
    if(m_isActive)
        nextFilter().MakeShadow(picfile, shadowfile, pList);
}

RtVoid OnOffFilter::MakeOcclusion(const StringArray& picfiles,
                                  RtConstString shadowfile,
                                  const ParamList& pList)
{
    if(m_isActive)
        nextFilter().MakeOcclusion(picfiles, shadowfile, pList);
}

RtVoid OnOffFilter::ErrorHandler(RtErrorFunc handler)
{
    if(m_isActive)
        nextFilter().ErrorHandler(handler);
}

RtVoid OnOffFilter::ReadArchive(RtConstToken name, RtArchiveCallback callback,
                                const ParamList& pList)
{
    if(m_isActive)
        nextFilter().ReadArchive(name, callback, pList);
}

RtVoid OnOffFilter::ArchiveBegin(RtConstToken name, const ParamList& pList)
{
    if(m_isActive)
        nextFilter().ArchiveBegin(name, pList);
}

RtVoid OnOffFilter::ArchiveEnd()
{
    if(m_isActive)
        nextFilter().ArchiveEnd();
}
//[[[end]]]

} // namespace Aqsis

// vi: set et:
