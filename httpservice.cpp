#include <httpservice.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
using namespace bbt;
using namespace std;
#define SOCKET_COMPLETION_DEFERRED false
#define SOCKET_COMPLETION_IMMEDIATE true


HttpSession::~HttpSession()
{
	// terminate service thread
	//delete service;
	
    SocketService *svc=service;

	// use unlock version detach 
	// because this  one is called in run()function
	// it has already hold lock
	detach_i();
	httpServer->freeService(svc);

	DEBUGLOG("HttpSession deleted !");
	

}

// When you create a HttpSession it waits to accept a
// connection.  This is done by it's own
HttpSession::HttpSession(
	HttpServer      * server,
	SocketService	* svc
   	) 
{
	DEBUGLOG("HttpSession Created");

	httpServer=server;
	request.setSession(this);
	response.setSession(this);
	tpport_t port;

	// Set up nonblock reads
	setCompletion( SOCKET_COMPLETION_DEFERRED);
	sessionState = SESSION_INITIAL;

	init( svc, (TCPSocket &)(*server) );

	InetHostAddress ia = getPeer( & port );

	cerr << "connecting from " << ia.getHostname() <<
		":" << port << endl;
}

    //
    // This is called by the SocketService thread when it the
    // object has expired.
    //

void HttpSession::expired()
{
	// Get outa here - this guy is a LOOSER - type or terminate

	DEBUGLOG("HttpSession Expired");
	delete this;
}

// This is called by the SocketService thread when it detects
// that there is somthing to read on this connection.
//

void HttpSession::pending()
{
	// Implement the echo
	
	char buf[size_of_buf];

	try {
    	int    len;

   	 	//while(len = Recv(buf, sizeof(buf))){ 
		if(sessionState == SESSION_INITIAL)
		{
			sessionState = SESSION_GETTINGHEAD;
		}
		if(sessionState == SESSION_GETTINGHEAD)
		{
			while(sessionState != SESSION_KNOWNHEAD)	
			{
				len=readLine(buf,sizeof(buf),0);	
				
				if(len <= 0)
				{
				   	return ;
				}
				//if(len==0) break;  //end of file
				if(len == 1) //  Null line is the end of head
				{
					sessionState = SESSION_PARSINGHEAD;
					request.parseRequest();
					sessionState = SESSION_KNOWNHEAD;
				}
				else
					request.append(buf);
			}
		}

		/*
		if(sessionState == SESSION_GETTINGBODY){
			len=readData(buf,sizeof(buf),0);
			if(len <= 0 ){
			   sessionsem->post();
		   	   return ;
			}
			request.append(buf,len);
		}
		*/
		
#ifdef DEBUG	
		if(sessionState == SESSION_KNOWNHEAD )
		{
			cout<<"Parsed Head"<<endl;
			cout<<request.getMethod()<<endl;
			cout<<request.getURI()<<endl;
			cout<<request.getParameter()<<endl;
			cout<<request.getConnection()<<endl;
			HttpMap::const_iterator it;
			HttpMap::const_iterator ie=request.getHeader().end();
			for(it=request.getHeader().begin();it!=ie;it++){
				cout<<it->first<<"="<<it->second<<endl;
			}
			
		}
#endif

		response.process();

		sessionState=SESSION_INITIAL;

		/*Keep alive or close immediately*/
		long time=request.getKeepAlive();
		if(time > 0)
			setTimer(time*1000l);
		else 
			delete this;
	}
	catch ( ... )
	{
    	// somthing wrong happened here !
		cerr << "Socket port read sent an exception !\n";
	}
}
	


void HttpSession::output(void)
{
	DEBUGLOG("Output ready");
}

void HttpSession::disconnect()
{
	// Called by the SocketService thread when the client
	// hangs up.
	DEBUGLOG("HttpSession disconnected!");

	delete this;
}



CCExec::CCExec()
{
}


//
// HttpServer simply creates HttpSession all the time until
// it has an error.  I suspect you could create as many of these
// as the OS could take.

const string HttpServer::defaultMimeType = "text/plain";
const string HttpServer::defaultConnection = "close";
const long HttpServer::defaultKeepAlive=100;
void HttpServer::run ()
{
	while ( 1 ) {
		try
		{
			// new does all the work to accept a new connection
			// attach itself to the SocketService 
			new HttpSession( this,getService());
		}
		catch ( ... )
		{
			// Bummer - there was an error.
			cerr << "HttpSession create failed\n";
			exit();
		}
	}
}

HttpServer::HttpServer(InetHostAddress & machine,
	int port,int service) : TCPSocket( machine, port ),Thread(),
	services(service)
{
	root="/home/sam/works/cpp/cpp11";
	defaultFile="index.html";
	initmime();
	this->start();
}

char * HttpServer::replace(char *str,char oldv,char newv)
{
	int len=strlen(str);
	int i;
	for(i=0;i<len;i++)
		if(str[i]==oldv) str[i]=newv;
	return str;
}

char * HttpServer::trim(char * str)
{
	//trim 
	int len=strlen(str);
	int i;
	char *tmp=str;
	for(i=0;i<len;i++){
		if(str[i]!=' '&&str[i]!='\t') break;
		tmp++;
	}
	len=strlen(tmp)-1;
	for(len;len>=0;len--)
		if(tmp[len]!=' '&&tmp[len]!='\t') break;
	for(i=0;i<=len;i++)
		str[i]=tmp[i];
	str[len+1]='\0';
	return str;
}
		
void HttpServer::initmime()
{
	ifstream mimefile;
	char path[256];
	char buf[256];
	char *line,*value,*avalue;
	char *last = NULL;
	string mime;
	string filename=root + "/conf/mimetype.conf";

	HttpMap::const_iterator it;

	mimefile.open(filename.c_str());
	if(!mimefile.is_open()){
		cerr<<"miss mime type file"<<endl;
		exit();
	}

	while(mimefile.getline(buf,sizeof(buf)))
	{
		line=buf;
		if (strlen(line) > 0)
		{
			/* get rid of spaces around the string, 
			   replace all tabs with spaces */
			line=replace(line,'\t',' ');
			line=trim(line);
			if (strlen(line) > 0 && line[0] != '#')
			{	
				value =strchr(line,' ');
				if (value !=NULL)
				{
					*(value++)=0;
					mime =string(line);
					value = trim(value);
					avalue=strtok_r(value," ",&last);
					while(avalue!=NULL)
					{

						mimeTypes.insert(pair<const string,string>(string(avalue),mime));
						avalue=strtok_r(NULL," ",&last);
					}

				}
			}
		}
	}
	mimefile.close();
}	

const string& HttpServer::getMimeType(const string& ext) const
{
	HttpMap::const_iterator it;
	it=mimeTypes.find(ext);
	if(it!=mimeTypes.end()) {
		return it->second;
	}
	return defaultMimeType;
}

HttpServer::~HttpServer()
{
}	
//
// Bug here, this should go ahead and shut down all sessions
// for application.  An exercise left to the reader.

CCExec::~CCExec()
{
    // MUST delete http first or it may end up using
    // a deleted service.
    if ( http ) delete http;
}

//
// Run App would normally read some config file or take some
// parameters about which port to connect to and then
// do that !
int CCExec::RunApp( const char * hn )
{
	// which port ?

	InetHostAddress machine( hn );

	if ( machine.isInetAddress() == false ) 
	{
		cerr << "machine is not address" << endl;
	}

	cerr << "machine is " << machine.getHostname() << endl;

	// Start accepting connections - this will bind to the
	// port as well.
	try 
	{
		http = new HttpServer(machine,8080,10);
	}
	catch ( ... )
	{
		cerr <<"Failed to bind\n"<<endl;
		return false;
	}

	return true;

}

// When there is no one else connected - terminate !

int main( int argc, char ** argv )
{
	CCExec	* server;

	server = new CCExec();

	// take the first command line option as a hostname
	// to listen to.
	if ( argc > 1 ) {
		server->RunApp( argv[ 1 ] );
	}
	else
	{
		server->RunApp();
	}
	server->mainsem->wait();


	delete server;

	return 0;
}
