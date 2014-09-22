/* comObjLibExt.h - VxCOM Embeded Extensions to comObjLib.h */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*

modification history
--------------------
01d,03jan02,nel  Remove references to T2OLE and OLE2T.
01c,09aug01,nel  Add wide string conversion.
01b,07aug01,nel  Add extra methods to VxComBSTR to allow creation from numeric
                 values.
01a,23jul01,nel  created

*/

/*

  DESCRIPTION:

  This file provides VxWorks specific extensions to the existing ATL like
  classes defined in comObjLib.h

*/


#ifndef __INCcomObjLibExt_h__
#define __INCcomObjLibExt_h__

#include <stdio.h>
#include <stdlib.h>

class VxComBSTR : public CComBSTR
    {
  public:

    VxComBSTR () : CComBSTR ()
        {
        m_text = NULL;
        }

    explicit VxComBSTR (int nSize, LPCOLESTR sz = 0) : CComBSTR (nSize, sz) 
	{
	m_text = NULL;
	}

    explicit VxComBSTR (const char * pstr) : CComBSTR (pstr) 
	{
	m_text = NULL;
	}

    explicit VxComBSTR (LPCOLESTR psz) : CComBSTR (psz)
	{
	m_text = NULL;
	}

    explicit VxComBSTR (const CComBSTR& src) : CComBSTR (src)
	{
	m_text = NULL;
	}

    explicit VxComBSTR (DWORD src) : CComBSTR ()
        {
        m_text = NULL;
	*this = src;
        }

    explicit VxComBSTR (DOUBLE src) : CComBSTR ()
        {
        m_text = NULL;
        *this = src;
        }

    ~VxComBSTR ()
        {
        if (m_text != NULL)
	    {
	    delete []m_text;
	    m_text = NULL;
	    }
        }
    
    operator char * ()
	{
	char *		ptr;

	if (m_text != NULL)
	    {
	    delete []m_text;
            m_text = NULL;
	    }

        if ((BSTR)(*this) == NULL)
            {
            return NULL;
            }

#ifdef _WIN32
	m_text = new char [wcslen ((BSTR)(*this)) + 1];
#else
	m_text = new char [comWideStrLen ((BSTR)(*this)) + 1];
#endif
	if (m_text == NULL) return NULL;
#ifdef _WIN32
	wcstombs (m_text, (BSTR)(*this), wcslen ((BSTR)(*this)) + 1);
#else
	comWideToAscii (m_text, (BSTR)(*this), comWideStrLen ((BSTR)(*this)) + 1);
#endif
	return m_text;
	}

    operator DWORD ()
        {
        return (DWORD)atol (*this);
        }

    VxComBSTR& operator = (const DWORD& src)
        {
        if (m_text)
            {
            delete []m_text;
            m_text = NULL;
            }
        char buffer [32];
	sprintf (buffer, "%ld", src);
	*this = buffer;
	return *this;
        }

    void SetHex (const DWORD src)
	{
        if (m_text)
            {
            delete []m_text;
            m_text = NULL;
            }
        char buffer [32];
	sprintf (buffer, "%lX", src);
	*this = buffer;
	}

    VxComBSTR& operator = (const DOUBLE& src)
        {
        if (m_text)
            {
            delete []m_text;
            m_text = NULL;
            }
        char buffer [32];
	sprintf (buffer, "%f", src);
	*this = buffer;
	return *this;
        }

    VxComBSTR& operator = (const char * src)
        {
        if (m_text)
            {
            delete []m_text;
            m_text = NULL;
            }
	OLECHAR * wStr = new OLECHAR [(strlen (src) + 1)];
#ifdef _WIN32
	mbstowcs (wStr, src, strlen (src) + 1);
#else
	comAsciiToWide (wStr, src, strlen (src) + 1);
#endif
	*((CComBSTR *)this) = wStr;
	delete []wStr;
	return *this;
        }

    bool const operator == (const VxComBSTR& src)
        {
        long int	i;

        if (Length () != src.Length ())
            {
            return false;
            }

        for (i = 0; i < (long int)Length (); i++)
            {
            if (((BSTR)(*this)) [i] != ((BSTR)src)[i])
                {
                return false;
                }
            }
        return true;
        }

    bool const operator != (const VxComBSTR& src)
        {
        return !(*this == src);
        }

  private:

    char *	m_text;
    };

#endif




