#if !defined(AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_)
#define AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

int ConvertAcmWav(int fhandle, long maxlen, unsigned char *&memory, long &samples_written, int forcestereo);
int ConvertWavAcm(int fh, long maxlen, FILE *foutp, bool wavc_or_acm);

#endif // !defined(AFX_ACMSOUND_H__C975A46B_7D3C_433C_8F35_9582A94DEF15__INCLUDED_)