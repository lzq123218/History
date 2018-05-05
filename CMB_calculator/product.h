#ifndef CAL_PRODUCT
#define CAL_PRODUCT

#include "atltime.h"
class Product{

public:
    Product(){};
    ~Product(){};

    bool SetDate( CTime start, CTime end, CTime harvest );
    bool SetRatio( double ratio );
    bool SetBaseDays( INT base_days );
    bool Calculate( CStringW& output);
    bool GetErrorMessage( CStringW& message );

    int GetValidDays();

private:
    INT m_BaseDays;
    CTime m_Start;
    CTime m_End;
    CTime m_Harvest;

    double m_Ratio;
    
};

#endif