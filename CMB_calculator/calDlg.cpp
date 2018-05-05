
// calDlg.cpp : implementation file
//

#include "stdafx.h"
#include "cal.h"
#include "calDlg.h"
#include "afxdialogex.h"
#include "string.h"
#include "parser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CcalDlg dialog

CcalDlg::CcalDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CcalDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CcalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CcalDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_EN_CHANGE(IDC_EDIT_RATIO, &CcalDlg::OnEnChangeEditRatio)
    ON_EN_CHANGE(IDC_EDIT_DAYS, &CcalDlg::OnEnChangeEditDays)
    ON_BN_CLICKED(IDC_BUTTON_CLEAN, &CcalDlg::OnBnClickedButtonClean)
    ON_BN_CLICKED(IDC_BUTTON_CAL, &CcalDlg::OnBnClickedButtonCal)
    ON_BN_CLICKED(IDC_RADIO_365, &CcalDlg::OnBnClickedRadio365)
    ON_BN_CLICKED(IDC_RADIO_360, &CcalDlg::OnBnClickedRadio360)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_START, &CcalDlg::OnDtnDatetimechangeDatetimepickerStart)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_END, &CcalDlg::OnDtnDatetimechangeDatetimepickerEnd)
    ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_START2, &CcalDlg::OnDtnDatetimechangeDatetimepickerStart2)
    ON_EN_CHANGE(IDC_EDIT_PRODUCT_CODE, &CcalDlg::OnEnChangeEditProductCode)
    ON_BN_CLICKED(IDC_BUTTON_GET, &CcalDlg::OnBnClickedButtonGet)
END_MESSAGE_MAP()


// CcalDlg message handlers

BOOL CcalDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

    m_pStart = reinterpret_cast<CDateTimeCtrl *> (GetDlgItem( IDC_DATETIMEPICKER_START ));
    m_pEnd = reinterpret_cast<CDateTimeCtrl *> (GetDlgItem( IDC_DATETIMEPICKER_END ));
    m_pHarvest = reinterpret_cast<CDateTimeCtrl *> (GetDlgItem( IDC_DATETIMEPICKER_START2 ));

    m_pDaysEditor = reinterpret_cast<CEdit *> (GetDlgItem( IDC_EDIT_DAYS ));
    m_pRatioEditor = reinterpret_cast<CEdit *> (GetDlgItem( IDC_EDIT_RATIO ));
    m_pCodeEditor = reinterpret_cast<CEdit *> (GetDlgItem( IDC_EDIT_PRODUCT_CODE ));

    m_pCodeEditor->SetLimitText( 6 );

    m_pResult = reinterpret_cast<CStatic *> (GetDlgItem( IDC_STATIC_RESULT ));

    OnBnClickedButtonClean();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CcalDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CcalDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CcalDlg::OnEnChangeEditRatio()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CcalDlg::OnEnChangeEditDays()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CcalDlg::OnBnClickedButtonClean()
{
    // TODO: Add your control notification handler code here
    m_BaseDays = 365;
    CButton* p365 = reinterpret_cast<CButton *> (GetDlgItem( IDC_RADIO_365 ));
    CButton* p360 = reinterpret_cast<CButton *> (GetDlgItem( IDC_RADIO_360 ));
    p365->SetCheck(1);
    p360->SetCheck(0);

    CTime tmp = CTime::GetCurrentTime();
    mStart = tmp;
    mEnd = tmp;
    mCommence = tmp;

    m_pStart->SetTime(&tmp);
    m_pEnd->SetTime(&tmp);
    m_pHarvest->SetTime(&tmp);

    m_pDaysEditor->SetWindowText(L"");
    m_pRatioEditor->SetWindowText(L"");
    m_pResult->SetWindowText(L"input your value");

}


void CcalDlg::OnBnClickedButtonCal()
{
    // TODO: Add your control notification handler code here
    if ( m_pRatioEditor->GetWindowTextLength() && m_pDaysEditor->GetWindowTextLength() ){

        if ( mEnd >= mStart ){

            if ( mCommence > mEnd ){

                CStringW sRatio;
                m_pRatioEditor->GetWindowText( sRatio);
                double ratio =_tstof(sRatio);
                CStringW Output;

                Product choice;
                choice.SetBaseDays( m_BaseDays );
                choice.SetDate( mStart, mEnd, mCommence );
                choice.SetRatio( ratio );

                if ( choice.Calculate( Output ) ){

                    m_pResult->SetWindowText(Output);
                }
            }

        }else{

            m_pResult->SetWindowText(L"Invalid timespan");            
        }

    }else{

        m_pResult->SetWindowText(L"Days or Ration is wrong");
    }

}


void CcalDlg::OnBnClickedRadio365()
{
    // TODO: Add your control notification handler code here
    m_BaseDays = 365;
}


void CcalDlg::OnBnClickedRadio360()
{
    // TODO: Add your control notification handler code here
    m_BaseDays = 360;
}


void CcalDlg::OnDtnDatetimechangeDatetimepickerStart(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
    // TODO: Add your control notification handler code here
    mStart = pDTChange->st;

    CheckTime();
    *pResult = 0;
}


void CcalDlg::OnDtnDatetimechangeDatetimepickerEnd(NMHDR *pNMHDR, LRESULT *pResult)
{
    *pResult = 0;

    LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
    // TODO: Add your control notification handler code here
    CTime tmp = pDTChange->st;

    if ( tmp < mStart ){

        m_pResult->SetWindowText(L"End date is wrong");

    }else{

        mEnd = tmp;
        CheckTime();
    }

}


void CcalDlg::OnDtnDatetimechangeDatetimepickerStart2(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;

    CTime tmp = pDTChange->st;

    if ( tmp < mEnd ){

        m_pResult->SetWindowText(L"End date is wrong");

    }else{

        mCommence = tmp;
        CheckTime();
    }
}

void CcalDlg::OnEnChangeEditProductCode()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
}


void CcalDlg::OnBnClickedButtonGet()
{
    // TODO: Add your control notification handler code here
    OnBnClickedButtonCal();
    CStringW codeW;
    m_pCodeEditor->GetWindowText( codeW );

    if ( codeW.GetLength() ){

        propertys out;   
        GetAndParse( codeW.GetBuffer(), out );
        CStringW keyW = L"产品代码";

        propertys::iterator it = out.find( keyW );

        if ( it != out.end() && it->second == codeW ){

            CStringW startW(L"发售起始日期");
            CStringW endW(L"发售截止日期");
            CStringW harvestW(L"产品到期日");

            startW = out[startW];
            endW = out[endW];
            harvestW = out[harvestW];

            int year, month, day;
            int res = 0;

            res = swscanf_s( startW.GetBuffer(), L"%d-%d-%d", &year, &month, &day );
            CTime start( year, month, day, 0, 0, 0, 0 );            
            mStart = start;
            m_pStart->SetTime(&start);
          
            res = swscanf_s( endW.GetBuffer(), L"%d-%d-%d", &year, &month, &day );
            CTime end( year, month, day, 0, 0, 0, 0 );
            mEnd = end;
            m_pEnd->SetTime(&end);

            res = swscanf_s( harvestW.GetBuffer(), L"%d-%d-%d", &year, &month, &day );
            CTime harvest( year, month, day, 0, 0, 0, 0 );
            mCommence = harvest;
            m_pHarvest->SetTime(&harvest);

            CheckTime();

            CStringW ratioW(L"收益率");

            ratioW = out[ratioW];
            double ratio = 0; 
            int index = ratioW.Find(L'%');
            if ( index >=0 ){

                ratioW.Delete( index, ratioW.GetLength() - index );
            }

            res = swscanf_s( ratioW.GetBuffer(), L"%f", &ratio );
            m_pRatioEditor->SetWindowText( ratioW );           
            
        }
    }
}

void CcalDlg::CheckTime(){

    Product tmp;

    if ( tmp.SetDate(mStart, mEnd, mCommence ) ){

        int Days = tmp.GetValidDays();

        if ( Days > 0 ){

            CStringW value;
            value.Format(L"%d",Days);
            m_pDaysEditor->SetWindowText( value );
        }

    }else{

        m_pResult->SetWindowText(L"date is wrong");
    }

}
