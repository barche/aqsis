// Aqsis
// Copyright � 1997 - 2001, Paul C. Gregory
//
// Contact: pgregory@aqsis.com
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


/** \file
		\brief Implements the CqImageBuffer class responsible for rendering the primitives and storing the results.
		\author Paul C. Gregory (pgregory@aqsis.com)
*/

#include	"aqsis.h"

#ifdef WIN32
#include    <windows.h>
#endif
#include	<math.h>

#include	"options.h"
#include	"renderer.h"
#include    "random.h"
#include	"imagepixel.h"
#include	"logging.h"


START_NAMESPACE( Aqsis )

//----------------------------------------------------------------------
/** Constructor
 */

CqImagePixel::CqImagePixel() :
        m_XSamples( 0 ),
        m_YSamples( 0 ),
        m_MaxDepth( FLT_MAX ),
        m_MinDepth( FLT_MAX ),
        m_OcclusionBoxId( -1 ),
        m_NeedsZUpdate( TqFalse )
{}


//----------------------------------------------------------------------
/** Destructor
 */

CqImagePixel::~CqImagePixel()
{}


//----------------------------------------------------------------------
/** Copy constructor
 */

CqImagePixel::CqImagePixel( const CqImagePixel& ieFrom )
{
    *this = ieFrom;
}


//----------------------------------------------------------------------
/** Allocate the subpixel samples array.
 * \param XSamples Integer samples count in X.
 * \param YSamples Integer samples count in Y.
 */

void CqImagePixel::AllocateSamples( TqInt XSamples, TqInt YSamples )
{
    if( m_XSamples != XSamples || m_YSamples != YSamples )
    {
        m_XSamples = XSamples;
        m_YSamples = YSamples;
        TqInt numSamples = m_XSamples * m_YSamples;

        if ( XSamples > 0 && YSamples > 0 )
        {
            m_aValues.resize( numSamples );
            m_Samples.resize( numSamples );
        }
    }
}

//----------------------------------------------------------------------
/** Fill in the sample array usig the multijitter function from GG IV.
 * \param vecPixel Cq2DVector pixel coordinate of this image element, used to make sure sample points are absolute, not relative.
 * \param fJitter Flag indicating whether to apply jittering to the sample points or not.
 */

void CqImagePixel::InitialiseSamples( std::vector<CqVector2D>& vecSamples, TqBool fJitter )
{
    TqFloat opentime = QGetRenderContext() ->optCurrent().GetFloatOption( "System", "Shutter" ) [ 0 ];
    TqFloat closetime = QGetRenderContext() ->optCurrent().GetFloatOption( "System", "Shutter" ) [ 1 ];

    TqFloat subcell_width = 1.0f / ( m_XSamples * m_YSamples );
    TqInt m = m_XSamples;
    TqInt n = m_YSamples;
    TqInt i, j;

    vecSamples.resize(m_XSamples * m_YSamples);
    if ( !fJitter )
    {
        // Initialise the samples to the centre points.
        TqFloat XInc = ( 1.0f / m_XSamples ) / 2.0f;
        TqFloat YInc = ( 1.0f / m_YSamples ) / 2.0f;
        TqInt y;
        for ( y = 0; y < m_YSamples; y++ )
        {
            TqFloat YSam = YInc + ( YInc * y );
            TqInt x;
            for ( x = 0; x < m_XSamples; x++ )
                vecSamples[ ( y * m_XSamples ) + x ] = CqVector2D( XInc + ( XInc * x ), YSam );
        }


        // Fill in the sample times for motion blur, LOD and SubCellIndex entries

        TqFloat time = 0;
        TqInt nSamples = m_XSamples*m_YSamples;
        TqFloat dtime = 1.0f / nSamples;

        for ( i = 0; i < nSamples; i++ )
        {
            m_Samples[ i ].m_SubCellIndex = 0;
            m_Samples[ i ].m_DetailLevel = m_Samples[ i ].m_Time = time;
            time += dtime;
        }


    }
    else
    {
        // Initiliaze the random with a value based on the X,Y coordinate
        //		CqRandom random(  vecPixel.Magnitude()  );
        static CqRandom random(  53 );

        // Initialize points to the "canonical" multi-jittered pattern.

        for ( i = 0; i < n; i++ )
        {
            for ( j = 0; j < m; j++ )
            {
                TqInt which = i * m + j;
                vecSamples[which].x( i );
                vecSamples[which].y( j );
            }
        }

        // Shuffle y coordinates within each row of cells.
        for ( i = 0; i < n; i++ )
        {
            for ( j = 0; j < m; j++ )
            {
                TqFloat t;
                TqInt k;

                k = random.RandomInt( n - 1 - i ) + i;
                TqInt i1 = i * m + j;
                TqInt i2 = k * m + j;
                assert( i1 < vecSamples.size() && i2 < vecSamples.size() );
                t = vecSamples[ i1 ].y();
                vecSamples[ i1 ].y( vecSamples[ i2 ].y() );
                vecSamples[ i2 ].y( t );
            }
        }

        // Shuffle x coordinates within each column of cells.
        for ( i = 0; i < m; i++ )
        {
            for ( j = 0; j < n; j++ )
            {
                TqFloat t;
                TqInt k;

                k = random.RandomInt( n - 1 - j ) + j;
                TqInt i1 = j * m + i;
                TqInt i2 = k * m + i;
                assert( i1 < vecSamples.size() && i2 < vecSamples.size() );
                t = vecSamples[ i1 ].x();
                vecSamples[ i1 ].x( vecSamples[ i2 ].x() );
                vecSamples[ i2 ].x( t );

            }
        }


        TqFloat subpixelheight = 1.0f / m_YSamples;
        TqFloat subpixelwidth = 1.0f / m_XSamples;

        TqInt which = 0;
        for ( i = 0; i < n; i++ )
        {
            TqFloat sy = i * subpixelheight;
            for ( j = 0; j < m; j++ )
            {
                TqFloat sx = j * subpixelwidth;
                TqFloat xindex = vecSamples[ which ].x();
                TqFloat yindex = vecSamples[ which ].y();
                vecSamples[ which ].x( xindex * subcell_width + ( subcell_width * 0.5f ) + sx );
                vecSamples[ which ].y( yindex * subcell_width + ( subcell_width * 0.5f ) + sy );
                m_Samples[ which ].m_SubCellIndex = static_cast<TqInt>( ( yindex * m_YSamples ) + xindex );
                which++;
            }
        }


        // Fill in the sample times for motion blur, detail levels for LOD and DoF.

        TqInt nSamples = m_XSamples*m_YSamples;

        TqFloat time = 0;
        TqFloat dtime = 1.0f / nSamples;

        TqFloat lod = 0;
        TqFloat dlod = dtime;

        TqFloat r = 0, theta = 0;
        TqFloat dr = dtime;
        TqFloat dtheta = 2 * PI * dtime;

        // We have to be a little more careful to pick DoF samples because we must
        // sample them in polar coordinates to ensure uniform coverage.

        for ( i = 0; i < nSamples; i++ )
        {
            // Pick a lens position randomly in polar coordinates

            m_Samples[i].m_DofOffset.x( sqrt( r + random.RandomFloat(dr) ) );
            m_Samples[i].m_DofOffset.y( theta + random.RandomFloat( dtheta ) );
            r += dr;
            theta += dtheta;
        }

        for ( i = 0; i < nSamples; i++ )
        {
            // Scale the value of time to the shutter time.
            TqFloat t = time + random.RandomFloat( dtime );
            t = ( closetime - opentime ) * t + opentime;
            m_Samples[ i ].m_Time = t;
            time += dtime;

            m_Samples[ i ].m_DetailLevel = lod + random.RandomFloat( dlod );
            lod += dlod;

            // For DoF samples, shuffle in both coordinates, then convert from polar
            // back to cartesian

            TqInt j = random.RandomInt( nSamples - 1 - i ) + i;
            TqInt k = random.RandomInt( nSamples - 1 - i ) + i;

            CqVector2D& iDofOffset = m_Samples[ i ].m_DofOffset;
            CqVector2D& jDofOffset = m_Samples[ j ].m_DofOffset;
            CqVector2D& kDofOffset = m_Samples[ k ].m_DofOffset;
            r     = jDofOffset.x();
            theta = kDofOffset.y();

            jDofOffset.x( iDofOffset.x() );
            kDofOffset.y( iDofOffset.y() );

            iDofOffset.x( r * sin(theta) );
            iDofOffset.y( r * cos(theta) );
            // these hold the quadrant the offset falls into. it is used as a quick rejection test during sampling.
            // if the sample is (eg) to the left of the mpg bound and the x-quad is negative then the sample can
            // not hit, and we needn't work out it's exact position.
            m_Samples[ i ].m_DofOffsetXQuad = iDofOffset.x() < 0.0f ? -1 : 1;
            m_Samples[ i ].m_DofOffsetYQuad = iDofOffset.y() < 0.0f ? -1 : 1;
        }

    }
    m_Data.m_Data.resize( QGetRenderContext()->GetOutputDataTotalSize() );
}

//----------------------------------------------------------------------
/** Offset the precomputed sample locations for a particular pixel
 */

void CqImagePixel::OffsetSamples(CqVector2D& vecPixel, std::vector<CqVector2D>& vecSamples)
{
    // add in the pixel offset
    for ( TqInt i = 0; i < m_YSamples*m_XSamples; i++ )
    {
        m_Samples[ i ].m_Position = vecSamples[ i ] + vecPixel;
    }
}

//----------------------------------------------------------------------
/** Clear the relevant data from the image element preparing it for the next usage.
 */

void CqImagePixel::Clear()
{
    TqInt i;
    for ( i = ( m_XSamples * m_YSamples ) - 1; i >= 0; i-- )
        m_aValues[ i ].clear( );
    m_MaxDepth = FLT_MAX;
    m_MinDepth =  FLT_MAX;
    m_OcclusionBoxId = -1;
    m_NeedsZUpdate = TqFalse;

}


//----------------------------------------------------------------------
/** Get the color at the specified sample point by blending the colors that appear at that point.
 */

void CqImagePixel::Combine()
{
    TqFloat coverage = 0;
    TqInt depthfilter = 0;

    const CqString* pstrDepthFilter = QGetRenderContext() ->optCurrent().GetStringOption( "Hider", "depthfilter" );

    if ( NULL != pstrDepthFilter )
    {
        if( !pstrDepthFilter[ 0 ].compare( "min" ) )
            depthfilter = 0;
        else if ( !pstrDepthFilter[ 0 ].compare( "midpoint" ) )
            depthfilter = 1;
        else if ( !pstrDepthFilter[ 0 ].compare( "max" ) )
            depthfilter = 2;
        else if ( !pstrDepthFilter[ 0 ].compare( "average" ) )
            depthfilter = 3;
        else
            std::cerr << warning << "Invalid depthfilter \"" << pstrDepthFilter[ 0 ].c_str() << "\", depthfilter set to \"min\"" << std::endl;
    }



    TqUint samplecount = 0;
    TqUint numsamples = XSamples() * YSamples();
    std::vector<std::vector<SqImageSample> >::iterator end = m_aValues.end();
    for ( std::vector<std::vector<SqImageSample> >::iterator samples = m_aValues.begin(); samples != end; ++samples )
    {
        // Find out if any of the samples are in a CSG tree.
        TqBool bProcessed;
        TqBool CqCSGRequired = CqCSGTreeNode::IsRequired();
        if (CqCSGRequired)
            do
            {
                bProcessed = TqFalse;
                //Warning ProcessTree add or remove elements in samples list
                //We could not optimized the for loop here at all.
                for ( std::vector<SqImageSample>::iterator isample = samples->begin(); isample != samples->end(); ++isample )
                {
                    if ( isample->m_pCSGNode )
                    {
                        isample->m_pCSGNode->ProcessTree( *samples );
                        bProcessed = TqTrue;
                        break;
                    }
                }
            } while ( bProcessed );

        CqColor samplecolor = gColBlack;
        CqColor sampleopacity = gColBlack;
        TqBool samplehit = TqFalse;

        for ( std::vector<SqImageSample>::reverse_iterator sample = samples->rbegin(); sample != samples->rend(); sample++ )
        {
            if ( sample->m_flags & SqImageSample::Flag_Matte )
            {
                if ( sample->m_flags & SqImageSample::Flag_Occludes )
                {
                    // Optimise common case
                    samplecolor = gColBlack;
                    sampleopacity = gColBlack;
                }
                else
                {
                    samplecolor.SetColorRGB(
                        LERP( sample->Os().fRed(), samplecolor.fRed(), 0 ),
                        LERP( sample->Os().fGreen(), samplecolor.fGreen(), 0 ),
                        LERP( sample->Os().fBlue(), samplecolor.fBlue(), 0 )
                    );
                    sampleopacity.SetColorRGB(
                        LERP( sample->Os().fRed(), sampleopacity.fRed(), 0 ),
                        LERP( sample->Os().fGreen(), sampleopacity.fGreen(), 0 ),
                        LERP( sample->Os().fBlue(), sampleopacity.fBlue(), 0 )
                    );
                }
            }
            else
            {
                samplecolor = ( samplecolor * ( gColWhite - sample->Os() ) ) + sample->Cs();
                sampleopacity = ( ( gColWhite - sampleopacity ) * sample->Os() ) + sampleopacity;
            }
            samplehit = TqTrue;
        }

        if ( samplehit )
        {
            coverage += 1.0f;
            samplecount++;
        }

        // Write the collapsed color values back into the top entry.
        if ( samples->size() > 0 )
        {
            samples->begin() ->SetCs( samplecolor );
            samples->begin() ->SetOs( sampleopacity );

            if ( depthfilter != 0)
            {
                if ( depthfilter == 1 )
                {
                    // Use midpoint for depth
                    if ( samples->size() > 1 )
                        ( *samples ) [ 0 ].SetDepth( ( ( *samples ) [ 0 ].Depth() + ( *samples ) [ 1 ].Depth() ) * 0.5f );
                    else
                        ( *samples ) [ 0 ].SetDepth( FLT_MAX );
                }
                else if ( depthfilter == 2)
                {
                    ( *samples ) [ 0 ].SetDepth( samples->back().Depth() );
                }
                else if ( depthfilter == 3 )
                {
                    std::vector<SqImageSample>::iterator sample;
                    TqFloat totDepth = 0.0f;
                    for ( sample = samples->begin(); sample != samples->end(); sample++ )
                        totDepth += sample->Depth();
                    totDepth /= samples->size();

                    ( *samples ) [ 0 ].SetDepth( totDepth );
                }
                // Default to "min"
            }
        }
    }

    if ( samplecount )
        SetCoverage(coverage /= numsamples);
}

//----------------------------------------------------------------------
/** ReCalculate the min and max z values for this pixel
 */

void CqImagePixel::UpdateZValues()
{
    float currentMax = 0.0f;
    float currentMin = FLT_MAX;
    TqInt sx, sy;
    for ( sy = 0; sy < m_YSamples; sy++ )
    {
        for ( sx = 0; sx < m_XSamples; sx++ )
        {
            std::vector<SqImageSample>& aValues = m_aValues[ sy * m_XSamples + sx ];

            if ( aValues.size() > 0 )
            {
                std::vector<SqImageSample>::iterator sc = aValues.begin();
                // find first opaque sample
                while ( sc != aValues.end() && ( ! ( sc->m_flags & SqImageSample::Flag_Occludes ) || ( sc->m_pCSGNode ) ) )
                    sc++;
                if ( sc != aValues.end() )
                {
                    if ( sc->Depth() > currentMax )
                    {
                        currentMax = sc->Depth();
                    }
                    if ( sc->Depth() < currentMin )
                    {
                        currentMin = sc->Depth();
                    }
                }
                else
                {
                    currentMax = FLT_MAX;
                }
            }
            else
            {
                currentMax = FLT_MAX;
            }
        }
    }

    m_MaxDepth = currentMax;
    m_MinDepth = currentMin;
}

//---------------------------------------------------------------------

END_NAMESPACE( Aqsis )
