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
		\brief Declares the base CqRenderer class which is the central core of the rendering main loop.
		\author Paul C. Gregory (pgregory@aqsis.com)
*/

//? Is renderer.h included already?
#ifndef RENDERER_H_INCLUDED 
//{
#define RENDERER_H_INCLUDED 1

#include	<vector>
#include	<iostream>
#include	<time.h>

#include	"aqsis.h"

#include	"ri.h"
#include	"graphicsstate.h"
#include	"stats.h"
#include	"vector2d.h"
#include	"semaphore.h"
//#include	"messages.h"
#include	"shaders.h"
#include	"symbols.h"
#include	"iddmanager.h"
#include	"irenderer.h"
#include	"ilog.h"

START_NAMESPACE( Aqsis )

class CqImageBuffer;

struct SqCoordSys
{
	SqCoordSys( const char* strName, const CqMatrix& matToWorld, const CqMatrix& matWorldTo ) :
			m_matWorldTo( matWorldTo ),
			m_matToWorld( matToWorld ),
			m_strName( strName )
	{ m_hash = CqParameter::hash((char *) strName);}
	SqCoordSys()
	{}

	CqMatrix	m_matWorldTo;
	CqMatrix	m_matToWorld;
	CqString	m_strName;
	TqUlong         m_hash;
};

enum EqCoordSystems
{
    CoordSystem_Camera = 0,
    CoordSystem_Current,
    CoordSystem_World,
    CoordSystem_Screen,
    CoordSystem_NDC,
    CoordSystem_Raster,

    CoordSystem_Last,
};

enum EqRenderMode
{
    RenderMode_Image = 0,

    RenderMode_Shadows,
    RenderMode_Reflection,
};


//----------------------------------------------------------------------
/** \class  CqRenderer
 * The main renderer control class.  Contains all information relating to
 * the current image being rendered.  There is only ever one of these,
 * statically defined in the CPP file for this class, and globally available.
 */

class CqRenderer;
extern CqRenderer* pCurrRenderer;

class CqRenderer : public IqRenderer
{
	public:
		CqRenderer();
		virtual	~CqRenderer();

		virtual	CqModeBlock*	BeginMainModeBlock();
		virtual	CqModeBlock*	BeginFrameModeBlock();
		virtual	CqModeBlock*	BeginWorldModeBlock();
		virtual	CqModeBlock*	BeginAttributeModeBlock();
		virtual	CqModeBlock*	BeginTransformModeBlock();
		virtual	CqModeBlock*	BeginSolidModeBlock( CqString& type );
		virtual	CqModeBlock*	BeginObjectModeBlock();
		virtual	CqModeBlock*	BeginMotionModeBlock( TqInt N, TqFloat times[] );

		virtual	void	EndMainModeBlock();
		virtual	void	EndFrameModeBlock();
		virtual	void	EndWorldModeBlock();
		virtual	void	EndAttributeModeBlock();
		virtual	void	EndTransformModeBlock();
		virtual	void	EndSolidModeBlock();
		virtual	void	EndObjectModeBlock();
		virtual	void	EndMotionModeBlock();

		virtual	CqOptions&	optCurrent() const;
		virtual	const CqAttributes*	pattrCurrent();
		virtual	CqAttributes*	pattrWriteCurrent();
		virtual	const CqTransform*	ptransCurrent();
		virtual	CqTransform*	ptransWriteCurrent();

		virtual	TqFloat	Time() const;
		virtual	void	AdvanceTime();

		/** Set a pointer to the current context.
		 * Primarily for Procedural objects
		 * \return Pointer to a previous CqModeBlock.
		 */
		virtual	CqModeBlock*	pconCurrent(CqModeBlock* pcon )
		{
			CqModeBlock* prev = m_pconCurrent;
			m_pconCurrent = pcon;
			return ( prev );
		}
		/** Get a pointer to the current context.
		 * \return Pointer to a CqModeBlock derived class.
		 */
		virtual	CqModeBlock*	pconCurrent()
		{
			return ( m_pconCurrent );
		}
		/** Get a erad only pointer to the current context.
		 * \return Pointer to a CqModeBlock derived class.
		 */
		virtual const	CqModeBlock*	pconCurrent() const
		{
			return ( m_pconCurrent );
		}
		/** Get a pointer to the current image buffer.
		 * \return A CqImageBuffer pointer.
		 */
		virtual CqImageBuffer* pImage()
		{
			return ( m_pImageBuffer );
		}
		/** Set the pointer to the current image buffer.
		 */
		virtual	void	SetImage( CqImageBuffer* pImage )
		{
			m_pImageBuffer = pImage;
		}

		// Handle various coordinate system transformation requirements.
		virtual	CqMatrix	matSpaceToSpace	( const char* strFrom, const char* strTo, const CqMatrix& matShaderToWorld = CqMatrix(), const CqMatrix& matObjectToWorld = CqMatrix(), TqFloat time = 0.0f );
		virtual	CqMatrix	matVSpaceToSpace	( const char* strFrom, const char* strTo, const CqMatrix& matShaderToWorld = CqMatrix(), const CqMatrix& matObjectToWorld = CqMatrix(), TqFloat time = 0.0f );
		virtual	CqMatrix	matNSpaceToSpace	( const char* strFrom, const char* strTo, const CqMatrix& matShaderToWorld = CqMatrix(), const CqMatrix& matObjectToWorld = CqMatrix(), TqFloat time = 0.0f );

		virtual	const	TqFloat*	GetFloatOption( const char* strName, const char* strParam ) const;
		virtual	const	TqInt*	GetIntegerOption( const char* strName, const char* strParam ) const;
		virtual	const	CqString* GetStringOption( const char* strName, const char* strParam ) const;
		virtual	const	CqVector3D*	GetPointOption( const char* strName, const char* strParam ) const;
		virtual	const	CqColor*	GetColorOption( const char* strName, const char* strParam ) const;

		virtual	TqFloat*	GetFloatOptionWrite( const char* strName, const char* strParam );
		virtual	TqInt*	GetIntegerOptionWrite( const char* strName, const char* strParam );
		virtual	CqString* GetStringOptionWrite( const char* strName, const char* strParam );
		virtual	CqVector3D*	GetPointOptionWrite( const char* strName, const char* strParam );
		virtual	CqColor*	GetColorOptionWrite( const char* strName, const char* strParam );

		virtual	void	PrintString( const char* str )
		{
			std::cout << str;
		}

		virtual	IqTextureMap* GetTextureMap( const CqString& strFileName );
		virtual	IqTextureMap* GetEnvironmentMap( const CqString& strFileName );
		virtual	IqTextureMap* GetShadowMap( const CqString& strFileName );
		virtual	IqTextureMap* GetLatLongMap( const CqString& strFileName );

		virtual	TqBool	GetBasisMatrix( CqMatrix& matBasis, const CqString& name );


		/** Get a read only reference to the current transformation matrix.
		 * \return A constant reference to a CqMatrix.
		 */
		virtual const	CqMatrix&	matCurrent( TqFloat time = 0.0f ) const
		{
			return ( pconCurrent() ->matCurrent( time ) );
		}

		virtual	TqBool	SetCoordSystem( const char* strName, const CqMatrix& matToWorld );

		// Function which can be overridden by the derived class.
		virtual	void	Initialise();
		virtual	void	RenderWorld();

		virtual	void	AddDisplayRequest( const TqChar* name, const TqChar* type, const TqChar* mode, TqInt modeID, TqInt dataOffset, TqInt dataSize, std::map<std::string, void*>& mapOfArguments );
		virtual	void	ClearDisplayRequests();
		virtual	IqDDManager*	pDDmanager()
		{
			return ( m_pDDManager );
		}

		virtual	void	Quit();
		virtual	void	UpdateStatus()
		{}
		/** Get the global statistics class.
		 * \return A reference to the CqStats class on this renderer.
		 */
		virtual	CqStats&	Stats()
		{
			return ( m_Stats );
		}

		/**	Get the global log class instance
		 * @return	The pointer to IqLog
		 */
		virtual	IqLog*	Logger()
		{
			return( m_theLog );
		}

		// Contect change callbacks
		virtual	SqParameterDeclaration FindParameterDecl( const char* strDecl );
		virtual	void	AddParameterDecl( const char* strName, const char* strType );
		virtual	void	ClearSymbolTable()
		{
			m_Symbols.clear();
		}

		/** Get the list of currently registered shaders.
		 * \return A reference to a list of CqShaderRegister classes.
		 */
		virtual	IqShader* CreateShader( const char* strName, EqShaderType type );

		/** Flush any registered shaders.
		 */
		virtual	void	FlushShaders()
		{
			while ( m_Shaders.pFirst() != 0 )
				delete( m_Shaders.pFirst() );
		}

		/** Set the world to screen matrix.
		 * \param mat The new matrix to use as the world to screen transformation.
		 */
		virtual	void	SetmatScreen( const CqMatrix& mat )
		{
			m_aCoordSystems[ CoordSystem_Screen ].m_matWorldTo = mat;
		}
		/** Set the world to NDC matrix.
		 * \param mat The new matrix to use as the world to NDC transformation.
		 */
		virtual	void	SetmatNDC( const CqMatrix& mat )
		{
			m_aCoordSystems[ CoordSystem_NDC ].m_matWorldTo = mat;
		}
		/** Set the world to raster matrix.
		 * \param mat The new matrix to use as the world to raster transformation.
		 */
		virtual	void	SetmatRaster( const CqMatrix& mat )
		{
			m_aCoordSystems[ CoordSystem_Raster ].m_matWorldTo = mat;
		}
		/** Set the world to camera matrix.
		 * \param ptrans A pointer to the transformation object which represents the world to camera transform.
		 */
		virtual	void	SetmatCamera( const CqTransform* ptrans )
		{
			(*m_pTransCamera) = *ptrans;
			//m_aCoordSystems[ CoordSystem_Camera ] .m_matWorldTo =
			//m_aCoordSystems[ CoordSystem_Current ].m_matWorldTo = ptrans->GetMotionObjectInterpolated(0);
			//m_aCoordSystems[ CoordSystem_Camera ] .m_matToWorld =
			//m_aCoordSystems[ CoordSystem_Current ].m_matToWorld = ptrans->GetMotionObjectInterpolated(0).Inverse();
		}
		/** Get the current transformation stack.
		 * \return A reference to a vector of CqTransform class pointers.
		 */
		virtual std::vector<CqTransform*>&	TransformStack()
		{
			return ( m_TransformStack );
		}

		/** Set the lens data associated with depth of field effects.
		 * \param fstop The f-stop of the lens.
		 * \param focalLength The size of the lens.
		 * \param focalDistance The distance at which everything is in focus.
		 */
		void	SetDepthOfFieldData( TqFloat fstop, TqFloat focalLength, TqFloat focalDistance )
		{
			m_depthOfFieldData[0] = fstop;
			m_depthOfFieldData[1] = focalLength;
			m_depthOfFieldData[2] = focalDistance;
			if (fstop < FLT_MAX)
			{
			    m_depthOfFieldData[3] = 0.5 * (focalLength / fstop) * (focalDistance * focalLength) / (focalDistance - focalLength);
			}
		}

		/** Find out if we are using depth of field or not.
		 * \return TqTrue if depth of field is turned on.
		 */
		TqBool	UsingDepthOfField( ) const
		{
			return m_depthOfFieldData[0] < FLT_MAX;
		}

		/** Get the lens data associated with depth of field effects.
		 * \return The lens data.
		 */
		const TqFloat*	GetDepthOfFieldData( ) const
		{
			return m_depthOfFieldData;
		}

		void SetDepthOfFieldScale( TqFloat x, TqFloat y )
		{
			m_depthOfFieldScale[0] = x;
			m_depthOfFieldScale[1] = y;
		}

		TqFloat GetDepthOfFieldScaleX() const
		{
			return m_depthOfFieldScale[0];
		}

		TqFloat GetDepthOfFieldScaleY() const
		{
			return m_depthOfFieldScale[1];
		}

		void	RegisterShader( const char* strName, EqShaderType type, IqShader* pShader );
		CqShaderRegister* FindShader( const char* strName, EqShaderType type );

		struct SqOutputDataEntry
		{
			TqInt	m_Offset;
			TqInt	m_NumSamples;
		};
		TqInt	RegisterOutputData( const char* name );
		TqInt	OutputDataIndex( const char* name );
		TqInt	OutputDataSamples( const char* name );
		std::map<std::string, SqOutputDataEntry>& GetMapOfOutputDataEntries()
		{
			return( m_OutputDataEntries );
		}
		
		
		/** Get the number if samples needed to fulfil the display requests.
		 */
		TqInt	GetOutputDataTotalSize() const
		{
			return( m_OutputDataTotalSize );
		}

	virtual	void	SetCurrentFrame( TqInt FrameNo )
		{
			m_FrameNo = FrameNo;
		}
	virtual	TqInt	CurrentFrame() const
		{
			return( m_FrameNo );
		}

	private:
		CqModeBlock*	m_pconCurrent;					///< Pointer to the current context.
		CqStats	m_Stats;						///< Global statistics.
		CqAttributes*	m_pAttrDefault;					///< Default attributes.
		CqTransform*	m_pTransDefault;				///< Default transformation.
		CqImageBuffer*	m_pImageBuffer;					///< Pointer to the current image buffer.

		IqDDManager*	m_pDDManager;

		EqRenderMode	m_Mode;
		CqList<CqShaderRegister> m_Shaders;				///< List of registered shaders.
		TqBool	m_fSaveGPrims;
		std::vector<CqTransform*>	m_TransformStack;	///< The global transformation stack.
		CqTransform*	m_pTransCamera;					///< The camera transform.
		std::vector<SqParameterDeclaration>	m_Symbols;	///< Symbol table.

		TqFloat m_depthOfFieldData[4];	///< DoF data
		TqFloat m_depthOfFieldScale[2];

		void WhichMatWorldTo(CqMatrix &a, TqUlong thash);
		void WhichMatToWorld(CqMatrix &b, TqUlong thash);

		IqLog*			m_theLog;	///< Pointer to the global log.
		IqMessageTable*	m_theTable;	///< Pointer to the global message table, used by the log. 
		std::map<std::string, SqOutputDataEntry>	m_OutputDataEntries;
		TqInt	m_OutputDataOffset;
		TqInt	m_OutputDataTotalSize;

		CqOptions *m_pOptDefault;	///< Pointer to default options.
		TqInt	m_FrameNo;
	public:
		std::vector<SqCoordSys>	m_aCoordSystems;		///< List of reistered coordinate systems.
}
;


inline CqRenderer* QGetRenderContext()
{
	return ( pCurrRenderer );
}


void	QSetRenderContext( CqRenderer* pRenderer );


//-----------------------------------------------------------------------

END_NAMESPACE( Aqsis )

//}  // End of #ifdef RENDERER_H_INCLUDED
#endif
