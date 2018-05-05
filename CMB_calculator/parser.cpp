#include "stdafx.h"
#include "afxinet.h"
#include "parser.h"

using std::string;
using std::map;

void GetMaininfoFromChunk( string& input, propertys& out ){

    if ( input.length() ){

        string key, value;
        size_t end = 0;

        for(;;){

            size_t start = input.find( "<li>", end, 4 );

            if ( string::npos != start ){

                end = input.find( "<span", start, 5 );

                if ( string::npos != start && string::npos != end ){
    
                    key.assign( input, start+4, end - start -4 );
                }

                start = input.find( "bigBlue\">", end, 9 );

                if ( string::npos != start ){

                    end = input.find( "</span", start, 6 );
                }
                if ( string::npos != start && string::npos != end ){
    
                    value.assign( input, start+9, end - start -9 );
                }

                wchar_t outDst[512];
                DWORD len = 0;

                len = MultiByteToWideChar(CP_UTF8, 0, key.data(), key.length(), outDst, 512);
                CStringW keyW;

                if ( len ){

                    keyW.Append( outDst, len );
                    keyW.Remove( L'：' );
                }

                len = MultiByteToWideChar(CP_UTF8, 0, value.data(), value.length(), outDst, 512);
                CStringW valueW;

                if ( len ){

                    valueW.Append( outDst, len );

                    out.insert( value_pair(keyW, valueW) );
                }
                

            }else{
                break;
            }
        }
    }
}

int GetPrdlist( const char*p, size_t len, string& remain, propertys& out ){

    int ret = -1;
    string result,tmp;
    if( remain.length() ){

        tmp = remain;
    }
    tmp.append( p, len );

    size_t first = tmp.find( "prdlist", 0, 7 );
                
    if ( string::npos != first ){

        size_t start = tmp.find("<ul>", first, 4 );
        size_t end = tmp.find("</ul>", start, 5 );

        if ( string::npos != start && string::npos != end ){

            result.assign( tmp, start + 4, end - start - 4 );
            remain.assign( tmp, end + 5, tmp.length() - end -5 );

            GetMaininfoFromChunk( result, out );
            ret = 0;
        }else{

            remain.assign( tmp,first, tmp.length() - first );
        }
    }else{
        
        remain.empty();
    }
    
    return ret;
}

int GetRatio( const char*p, size_t len, string& remain, propertys& out ){
    int ret = -1;

    wchar_t *sigW = L"收益率为";
    char sig[64];
    memset( sig, 0, 64 );
    int sigLen = WideCharToMultiByte( CP_UTF8, 0, sigW, 4, sig, 64, NULL, NULL );

    string result,tmp;
    if( remain.length() ){

        tmp = remain;
    }
    tmp.append( p, len );

    size_t first = tmp.find( sig, 0, sigLen );
                
    if ( string::npos != first ){

        size_t start = tmp.find("yes\">", first, 5 );
        size_t end = tmp.find("</span", start, 6 );

        if ( string::npos != start && string::npos != end ){

            result.assign( tmp, start + 5, end - start - 5 );
            remain.assign( tmp, end + 6, tmp.length() - end -6 );

            wchar_t outDst[512];
            DWORD len = 0;

            len = MultiByteToWideChar(CP_UTF8, 0, result.data(), result.length(), outDst, 512);

            if ( len ){

                CStringW valueW( outDst, len );
                CStringW keyW(L"收益率");
                out.insert( value_pair(keyW, valueW) );            
            }

            ret = 0;
        }else{

            remain.assign( tmp,first, tmp.length() - first );
        }

    }else{
        remain.empty();
    }

    return ret;
}

int GetAndParse( const wchar_t* ProductCode, propertys& out ){

    //L"http://www.cmbchina.com/CFWEB/Personal/productdetail.aspx?code=310384&type=prodintro";

    CStringW urlW;
    urlW.Format(L"http://www.cmbchina.com/CFWEB/Personal/productdetail.aspx?code=%s&type=prodintro", ProductCode );
    CInternetSession session(L"HttpClient");

    CHttpFile* pfile = (CHttpFile *)session.OpenURL(urlW.GetBuffer());
    DWORD dwStatusCode;
    pfile->QueryInfoStatusCode(dwStatusCode);
    if(dwStatusCode == HTTP_STATUS_OK)
    {
        DWORD len = 0;       
        char pBuff[1024];
        PARSER_STATE state = PARSE_PRDLIST;
        string remain;
          
        do{
            len = pfile->Read(pBuff, 512);
            if ( len ){

                switch( state ){

                    case PARSE_PRDLIST:{

                        if ( GetPrdlist( pBuff, len, remain, out ) >= 0 ){
                            state = PARSE_RATIO;
                        }
                        break;
                    }
                    case PARSE_RATIO:{

                        if ( GetRatio( pBuff, len, remain, out ) >= 0 ){
                            state = PARSE_END;
                        }
                        break;
                    }
                    default:
                        break;
                }
                
            }
        }while ( len && PARSE_END != state );
    }
    pfile->Close();
    delete pfile;
    session.Close();

    return 0 ;
}