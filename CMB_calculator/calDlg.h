
// calDlg.h : header file
//

#pragma once

#include "parser.h"
#include "product.h"

// CcalDlg dialog
class CcalDlg : public CDialogEx
{
// Construction
public:
	CcalDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CAL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnEnChangeEditRatio();
    afx_msg void OnEnChangeEditDays();
    afx_msg void OnBnClickedButtonClean();
    afx_msg void OnBnClickedButtonCal();

private:
    CEdit *m_pDaysEditor;
    CEdit *m_pRatioEditor;
    CEdit *m_pCodeEditor;
    CStatic *m_pResult;

    CDateTimeCtrl *m_pStart;
    CDateTimeCtrl *m_pEnd;
    CDateTimeCtrl *m_pHarvest;

    INT m_BaseDays;
    CTime mStart;
    CTime mEnd;
    CTime mCommence;
    void CheckTime();

public:
    afx_msg void OnBnClickedRadio365();
    afx_msg void OnBnClickedRadio360();
    afx_msg void OnDtnDatetimechangeDatetimepickerStart(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDtnDatetimechangeDatetimepickerEnd(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnDtnDatetimechangeDatetimepickerStart2(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnEnChangeEditProductCode();
    afx_msg void OnBnClickedButtonGet();
};


