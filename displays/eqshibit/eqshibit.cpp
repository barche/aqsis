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
		\brief Implements the default display devices for Aqsis.
		\author Paul C. Gregory (pgregory@aqsis.com)
*/

#include <aqsis.h>

#include "argparse.h"

#include <logging.h>
#include <logging_streambufs.h>
#include "sstring.h"

#include <tiffio.h>

using namespace Aqsis;

#include <string>
#include <list>

#include "boost/archive/iterators/binary_from_base64.hpp"
#include "boost/archive/iterators/transform_width.hpp"
#include "boost/archive/iterators/remove_whitespace.hpp"

#include "boost/pfto.hpp"

#include <version.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "eqshibit.h"
#include "eqshibit_ui.h"
#include "ddserver.h"
#include "displayserverimage.h"
#include "framebuffer.h"
#include "book.h"
#include "tinyxml.h"


ArgParse::apstring      g_strInterface = "127.0.0.1";
ArgParse::apstring      g_strPort = "48515";
bool    		g_fHelp = 0;
bool    		g_fVersion = 0;
bool 			g_cl_no_color = false;
bool 			g_cl_syslog = false;
ArgParse::apint 	g_cl_verbose = 1;

CqEqshibitMainWindow *window = 0;

void version( std::ostream& Stream )
{
	Stream << "eqshibit version " << VERSION_STR_PRINT << std::endl << "compiled " << __DATE__ << " " << __TIME__ << std::endl;
}


typedef 
	boost::archive::iterators::transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
		boost::archive::iterators::binary_from_base64<    // convert binary values ot base64 characters
			boost::archive::iterators::remove_whitespace<
				std::string::const_iterator
			>
		>,
		8,
		6
	> 
base64_binary; // compose all the above operations in to a new iterator

CqDDServer g_theServer;
std::map<int, boost::shared_ptr<CqDisplayServerImage> >	g_theClients;
#define BUF_SIZE  4096

void ProcessMessage(std::stringstream& msg, boost::shared_ptr<CqDisplayServerImage> thisClient);

void HandleData(int sock, void *data)
{
	boost::shared_ptr<CqDisplayServerImage> thisClient = g_theClients[sock];
	char	buffer[BUF_SIZE];
	std::stringstream msg;
	char *bufPtr = buffer;
	size_t bufAvail = 0;
	bool noneWaiting = false;

	// Read a message
	while(1)	
	{
		// If there is more left in the buffer, process it.
		if(bufAvail > 0)
		{
			// Check if the buffer has a terminator.
			void* term;
			if((term = memchr(bufPtr, '\0', bufAvail)) != NULL)
			{
				// Copy the data up to the terminator.
				msg << bufPtr;
				// Point the bufPtr past the terminator, and update the avail count.
				bufAvail -= strlen(bufPtr)+1;
				bufPtr = reinterpret_cast<char*>(term);
				bufPtr++;
				ProcessMessage(msg, thisClient);
				msg.clear();
				if(noneWaiting)
					break;
			}
			else
			{
				msg << std::string(bufPtr, bufAvail);
				bufPtr = buffer;
				bufAvail = 0;
			}
		}
		if(bufAvail <= 0)
		{
			// Read some more into the buffer
			bufAvail = read(sock,buffer,BUF_SIZE);
			if(bufAvail < BUF_SIZE)
				noneWaiting = true;
			// If none available, exit.
			if(bufAvail <= 0) 
				break;
		}
	}
}

void ProcessMessage(std::stringstream& msg, boost::shared_ptr<CqDisplayServerImage> thisClient)
{
	// Parse the XML message sent.
	TiXmlDocument xmlMsg;
	xmlMsg.Parse(msg.str().c_str());
	// Get the root element, which is the base type of the message.
	TiXmlElement* root = xmlMsg.RootElement();

	if(root)
	{
		// Process the message based on its type.
		if(root->ValueStr().compare("Open") == 0)
		{
			Aqsis::log() << Aqsis::debug << "Processing Open message " << std::endl;
			TiXmlElement* child = root->FirstChildElement("Dimensions");
			if(child)
			{
				int xres = 640, yres = 480;
				child->Attribute("width", &xres);
				child->Attribute("height", &yres);
				thisClient->setImageSize(xres, yres);
			}
			
			child = root->FirstChildElement("Name");
			if(child)
			{
				const char* fname = child->GetText();
				thisClient->setName(fname);
			}
			// Process the parameters
			child = root->FirstChildElement("Parameters");
			if(child)
			{
				TiXmlElement* param = child->FirstChildElement("IntsParameter");
				while(param)
				{
					const char* name = param->Attribute("name");
					if(std::string("origin").compare(name) == 0)
					{
						TiXmlElement* values = param->FirstChildElement("Values");
						if(values)
						{
							int origin[2];
							TiXmlElement* value = values->FirstChildElement("Int");
							value->Attribute("value", &origin[0]);
							value = value->NextSiblingElement("Int");
							value->Attribute("value", &origin[1]);
							thisClient->setOrigin(origin[0], origin[1]);
						}
					}
					else if(std::string("OriginalSize").compare(name) == 0)
					{
						TiXmlElement* values = param->FirstChildElement("Values");
						if(values)
						{
							int OriginalSize[2];
							TiXmlElement* value = values->FirstChildElement("Int");
							value->Attribute("value", &OriginalSize[0]);
							value = value->NextSiblingElement("Int");
							value->Attribute("value", &OriginalSize[1]);
							thisClient->setFrameSize(OriginalSize[0], OriginalSize[1]);
						}
					}
					param = param->NextSiblingElement("IntsParameter");
				}
			}
			child = root->FirstChildElement("Formats");
			if(child)
			{
				int channels = 0;
				TiXmlElement* format = child->FirstChildElement("Format");
				while(format)
				{
					++channels;
					format = format->NextSiblingElement("Format");
				}
				thisClient->setChannels(channels);
			}
			thisClient->PrepareImageBuffer();

			boost::shared_ptr<CqImage> baseImage = boost::static_pointer_cast<CqImage>(thisClient);
			if(window->currentBook()->framebuffer())
			{
				window->currentBook()->framebuffer()->connect(baseImage);
				window->currentBook()->framebuffer()->show();
			}
			else
			{
				Aqsis::log() << Aqsis::debug << "Creating a new FB window" << std::endl;
				boost::shared_ptr<CqFramebuffer> fb(new CqFramebuffer(thisClient->imageWidth(), thisClient->imageHeight(), thisClient->channels()));
				window->currentBook()->setFramebuffer(fb);
				fb->show();
				fb->connect(baseImage);
			}
			window->updateImageList(window->currentBookName());
		}
		else if(root->ValueStr().compare("Data") == 0)
		{
			Aqsis::log() << Aqsis::debug << "Processing Data message " << std::endl;
			TiXmlElement* dimensionsXML = root->FirstChildElement("Dimensions");
			if(dimensionsXML)
			{
				int xmin, ymin, xmaxplus1, ymaxplus1, elementSize;
				dimensionsXML->Attribute("xmin", &xmin);
				dimensionsXML->Attribute("ymin", &ymin);
				dimensionsXML->Attribute("xmaxplus1", &xmaxplus1);
				dimensionsXML->Attribute("ymaxplus1", &ymaxplus1);
				dimensionsXML->Attribute("elementsize", &elementSize);
			
				TiXmlElement* bucketDataXML = root->FirstChildElement("BucketData");
				if(bucketDataXML)
				{
					TiXmlText* dataText = static_cast<TiXmlText*>(bucketDataXML->FirstChild());
					if(dataText)
					{
						TqInt bucketlinelen = elementSize * (xmaxplus1 - xmin);
						TqInt count = bucketlinelen * (ymaxplus1 - ymin);
						std::string data = dataText->Value();
						std::vector<unsigned char> binaryData;
						binaryData.reserve(count);
						base64_binary ti_begin = base64_binary(BOOST_MAKE_PFTO_WRAPPER(data.begin())); 
						std::size_t padding = 2 - count % 3;
						while(--count > 0)
						{
							binaryData.push_back(static_cast<char>(*ti_begin));
							++ti_begin;
						}
						binaryData.push_back(static_cast<char>(*ti_begin));
						if(padding > 1)
							++ti_begin;
						if(padding > 2)
							++ti_begin;
						thisClient->acceptData(xmin, xmaxplus1, ymin, ymaxplus1, elementSize, &binaryData[0]);
					}
				}
			}
		}
		else if(root->ValueStr().compare("Close") == 0)
		{
			Aqsis::log() << Aqsis::debug << "Closing socket" << std::endl;
			Fl::remove_fd(thisClient->socket());
			thisClient->close();
		}
	}		
}

void HandleConnection(int sock, void *data)
{
	Aqsis::log() << Aqsis::debug << "Connection established with display server" << std::endl;

	boost::shared_ptr<CqDisplayServerImage> newImage(new CqDisplayServerImage());
	newImage->setName("Unnamed");
	
	if(g_theServer.Accept(newImage))
	{
		g_theClients[newImage->socket()] = newImage;
		Fl::add_fd(newImage->socket(), FL_READ, &HandleData);
		if(window)
		{
			boost::shared_ptr<CqImage> baseImage = boost::static_pointer_cast<CqImage>(newImage);
			window->addImageToCurrentBook(baseImage);
		}
	}
}




int main( int argc, char** argv )
{
	// Create listening socket. 
	// Setup fltk. 
	// add the socket to the fltk event
	// run fltk
	// blah
	ArgParse ap;

	// Set up the options
	ap.usageHeader( ArgParse::apstring( "Usage: " ) + argv[ 0 ] + " [options]" );
	ap.argString( "i", "\aSpecify the address to listen on (default: " + g_strInterface + ")", &g_strInterface );
	ap.argString( "p", "\aSpecify the port to listen on (default: " + g_strPort + ")", &g_strPort );
	ap.argFlag( "help", "\aprint this help and exit", &g_fHelp );
	ap.argFlag( "version", "\aprint version information and exit", &g_fVersion );
	ap.argFlag( "nocolor", "\aDisable colored output", &g_cl_no_color );
	ap.alias( "nocolor", "nc" );
	ap.argInt( "verbose", "=integer\aSet log output level\n"
		"\a0 = errors\n"
		"\a1 = warnings (default)\n"
		"\a2 = information\n"
		"\a3 = debug", &g_cl_verbose );
	ap.alias( "verbose", "v" );

#ifdef  AQSIS_SYSTEM_POSIX
        ap.argFlag( "syslog", "\aLog messages to syslog", &g_cl_syslog );
#endif  // AQSIS_SYSTEM_POSIX


	const char **c_argv = const_cast<const char**>(argv);
	if ( argc > 1 && !ap.parse( argc - 1, c_argv + 1 ) )
	{
		std::cerr << ap.errmsg() << std::endl << ap.usagemsg();
 		exit( 1 );
	}

	if ( g_fVersion )
	{
		version( std::cout );
		exit( 0 );
	}

#ifdef  AQSIS_SYSTEM_WIN32
	std::auto_ptr<std::streambuf> ansi( new Aqsis::ansi_buf(std::cerr) );
#endif
	std::auto_ptr<std::streambuf> reset_level( new Aqsis::reset_level_buf(std::cerr) );
	std::auto_ptr<std::streambuf> show_timestamps( new Aqsis::timestamp_buf(std::cerr) );
	std::auto_ptr<std::streambuf> fold_duplicates( new Aqsis::fold_duplicates_buf(std::cerr) );
	std::auto_ptr<std::streambuf> color_level;
	if(!g_cl_no_color)
	{
		std::auto_ptr<std::streambuf> temp_color_level( new Aqsis::color_level_buf(std::cerr) );
		color_level = temp_color_level;
	}
	std::auto_ptr<std::streambuf> show_level( new Aqsis::show_level_buf(std::cerr) );
	Aqsis::log_level_t level = Aqsis::ERROR;
	if( g_cl_verbose > 0 )
		level = Aqsis::WARNING;
	if( g_cl_verbose > 1 )
		level = Aqsis::INFO;
	if( g_cl_verbose > 2 )
		level = Aqsis::DEBUG;
	std::auto_ptr<std::streambuf> filter_level( new Aqsis::filter_by_level_buf(level, Aqsis::log()) );

#ifdef  AQSIS_SYSTEM_POSIX
	if( g_cl_syslog )
		std::auto_ptr<std::streambuf> use_syslog( new Aqsis::syslog_buf(std::cerr) );
#endif  // AQSIS_SYSTEM_POSIX

	int portno = atoi(g_strPort.c_str());
	if(!g_theServer.Prepare(portno))
		Aqsis::log() << Aqsis::error << "Cannot open server on the specified port" << std::endl;

	Fl::add_fd(g_theServer.Socket(),&HandleConnection);

	window = new CqEqshibitMainWindow();
	char *internalArgs[] = {
		"eqshibit"
	};
	window->show(1, internalArgs);

	return Fl::run();
}

