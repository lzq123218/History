#include "stdafx.h"
#include "product.h"

bool Product::SetDate( CTime start, CTime end, CTime harvest ){
    bool ret(false);

    if ( start <= end ){

        if ( end < harvest ){

            m_Start = start;
            m_End = end;
            m_Harvest = harvest;
            ret = true;
        }        
    }
    return ret;
}

bool Product::SetRatio( double ratio ){
    bool ret(false);

    if ( ratio > 0 ){
        
        m_Ratio = ratio;
        ret = true;
    }
    return ret;
}

bool Product::SetBaseDays( int base_days ){
    bool ret(false);

    if ( base_days == 360 || base_days == 365){

        m_BaseDays = base_days;
        ret = true;
    }else{
        
    }

    return ret;
}

bool Product::Calculate( CStringW& output ){

    bool ret(false);
    CTimeSpan span = m_End - m_Start;
    LONGLONG days_preparation = span.GetDays() + 1;

    if ( m_Harvest > m_End ){

        CTimeSpan span = m_Harvest - m_End;
        DWORD nDays = GetValidDays();

        double result = 10000*nDays*m_Ratio/m_BaseDays/100.0;
            
        double actualRatio = 100*m_BaseDays*result/(days_preparation + nDays)/10000.0;

        output.Format(L"%s   %.2f   %.2f%%", m_Harvest.Format(L"%A, %B-%d-%Y"), result, actualRatio );
    
        ret = true;
    }
    return ret;
}

bool Product::GetErrorMessage( CStringW& message ){

    bool ret(false);

    return ret;
}

int Product::GetValidDays(){

    int ret = -1;

    if ( m_End >= m_Start ){

        if ( m_Harvest > m_End ){

            CTimeSpan span = m_Harvest - m_End;
            DWORD nDays = span.GetDays() - 1;
            ret = nDays;
        }
    }
    return ret;
}
