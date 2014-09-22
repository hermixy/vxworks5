/* unixRegistry.cpp - VxCOM Unix disk-based registry class */

/* Copyright (c) 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01b,03jan02,nel  Remove T2OLE reference.
01a,02oct01,dsellars  created
*/

#if defined (VXDCOM_PLATFORM_SOLARIS) || defined (VXDCOM_PLATFORM_LINUX)

#include <dirent.h>
#include <string>
#include <fstream>
#include <sys/param.h>
#include <dlfcn.h>
#include "comLib.h"
#include "comObjLib.h"
#include "private/comStl.h"
#include "private/comMisc.h"

//////////////////////////////////////////////////////////////////////////
//
// The 'FileRegistry' class implements a 'plug-in' VxCOM registry for
// Unix-based systems, where the coclasses are located in shared
// libraries on disk. Each shared library must export a function
// called DllGetClassObject() which must match the PFN_GETCLASSOBJECT
// signature.
//
// A text-file (with extension .registry) is used to record the CLSIDs
// and map them to shared libraries, which can be anywhere on the
// system. The registration file must be in a directory which is
// mentioned in the environment variable VXCOM_REGISTRY_PATH (which is
// a list of directories, separated by colons, in the usual Unix way).
//
// Each line in the registry file must be of the form:-
//
//  {CLSID}=/path/to/shared/library.so
//
// i.e. the string-form of the CLSID as defined by COM, followed by
// the path to the shared-library itself.
//

class FileRegistry :
    public CComObjectRoot,
    public IRegistry
    {
  public:
    virtual ~FileRegistry ();
    FileRegistry ();

    // IRegistry implementation...
    HRESULT RegisterClass (REFCLSID, void*)
        {
        // Not required for this kind of registry...
        return E_NOTIMPL;
        }
    
    HRESULT IsClassRegistered
        (
        REFCLSID                clsid
        );

    HRESULT CreateInstance
        (
        REFCLSID                clsid,
        IUnknown *              pUnkOuter,
        DWORD                   dwClsContext,
        const char *            hint,
        ULONG                   cMQIs,
        MULTI_QI *              pMQIs
        );

    HRESULT GetClassObject
        (
        REFCLSID                clsid,
        REFIID                  iid,
        DWORD                   dwClsContext,
        const char *            hint,
        IUnknown **             ppClsObj
        );

    HRESULT GetClassID
        (
        DWORD                   dwIndex,
        LPCLSID                 pclsid
        );        

    // Load a .registry file...
    int loadRegistryFile (const string& fileName);

    BEGIN_COM_MAP(FileRegistry)
        COM_INTERFACE_ENTRY(IRegistry)
    END_COM_MAP()

  private:

    // Not implemented
    FileRegistry (const FileRegistry& other);
    FileRegistry& operator= (const FileRegistry& rhs);

    // universal private instance-creation function
    HRESULT instanceCreate
        (
        bool                    classMode,
        REFCLSID                clsid,
        IUnknown *              pUnkOuter,
        DWORD                   dwClsContext,
        const char *            hint,
        ULONG                   cMQIs,
        MULTI_QI *              pMQIs
        );

    // registry entry
    struct entry
        {
        CLSID   clsid;
        string  library;

        entry (REFCLSID cid, const string& lib)
          : clsid (cid), library (lib)
            {}

        entry ()
            {}
        };
    
    typedef STL_MAP(CLSID, entry)       RegMap_t;

    RegMap_t    m_regmap;

    };

typedef CComObject<FileRegistry> FileRegistryClass;

//////////////////////////////////////////////////////////////////////////
//
// theRegistryInstance - returns the single instance of FileRegistryClass
//

static HRESULT theRegistryInstance (FileRegistryClass **ppReg)
    {
    static FileRegistryClass* s_instance = 0;

    if (! s_instance)
        {
        // Create one, add a ref to keep it around forever...
        s_instance = new FileRegistryClass ();
        if (! s_instance)
            return E_FAIL;
        
        s_instance->AddRef ();
        }

    *ppReg = s_instance;
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// comFileRegistryInit - initialise the Unix registry component
//
// This function reads all the registry files it can find, and adds
// entries to the one-and-only instance of FileRegistryClass. It then
// puts that registry into the VxCOM registry-list, so that classes
// are accessible to CoCreateInstance() via this mechanism.
//

extern "C" int comFileRegistryInit () 
    {
    IRegistry* pReg;

    FileRegistryClass* pur = 0;

    // Find the one-and-only instance (adds a ref)...
    HRESULT hr = theRegistryInstance (&pur);
    if (FAILED (hr))
        return hr;

    // Get its IRegistry interface...
    hr = pur->QueryInterface (IID_IRegistry, (void**) &pReg);
    if (FAILED (hr))
        return hr;

    // Add it to the VxCOM registry...
    hr = comRegistryAdd ("Unix Registry",
                         CLSCTX_INPROC_SERVER,
                         pReg);

    // Done with the IRegistry interface...
    pReg->Release ();

    // Get path list...
    const char* regPath = getenv ("VXCOM_REGISTRY_PATH");
    if (! regPath)
        return ERROR;

    // Split into dirs...
    string path (regPath);
    size_t prev = 0;
    size_t ix = 0;
    while (ix != string::npos)
        {
        ix = path.find_first_of (':', prev);
        string dirName = path.substr (prev, ix);
        if (ix != string::npos)
            prev = ix + 1;

        if (dirName.length () == 0)
            continue;

        // Now look in this dir for *.registry files...
        cout << "Searching directory " << dirName << endl;

        DIR* dir = opendir (dirName.c_str ());
        if (! dir)
            {
            perror (dirName.c_str ());
            continue;
            }

        struct dirent* pDirent;

        // Load them one by one...
        while ((pDirent = readdir (dir)) != 0)
            {
            const string DOT_REGISTRY = ".registry";
            
            string f = pDirent->d_name;
            if ((f.length () >= DOT_REGISTRY.length ()) &&
                (f.rfind (DOT_REGISTRY) == (f.length() - DOT_REGISTRY.length ())))
                {
                // Found a file...
                pur->loadRegistryFile (dirName + "/" + f);
                }
            }

        // Tidy up...
        closedir (dir);
        }

    // Clean up...
    pur->Release ();

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////

FileRegistry::FileRegistry ()
    {
    }


//////////////////////////////////////////////////////////////////////////

FileRegistry::~FileRegistry ()
    {
    }

//////////////////////////////////////////////////////////////////////////
//
// FileRegistry::loadRegistryFile - load a .registry file
//

int FileRegistry::loadRegistryFile (const string& fileName)
    {
    cout << "Loading registry file " << fileName << endl;

    ifstream str (fileName.c_str ());

    if (! str)
        return -1;

    char buf [MAXPATHLEN + 40];    // big enough for GUID + path

    while (str.getline (buf, sizeof (buf)))
        {
        string line = buf;

        size_t eq = line.find_first_of ('=');
        if (eq == string::npos)
            continue;
        
        string clsid = line.substr (0, eq);
        string shlib = line.substr (eq+1, string::npos);
        
        CLSID classid;
        OLECHAR * pwsz = new OLECHAR [(strlen (clsid.c_str ()) + 1)];

	comAsciiToWide (pwsz, clsid.c_str (), (strlen (clsid.c_str ()) + 1));
        HRESULT hr = CLSIDFromString (pwsz, &classid);
	delete []pwsz;
        if (FAILED (hr))
            return hr;

        cout << "Registering CLSID " << clsid << endl
             << "  for shared-lib " << shlib << endl;

        m_regmap [classid] = entry (classid, shlib);
        }
    
    return 0;
    }

//////////////////////////////////////////////////////////////////////////
//
// FileRegistry::IsClassRegistered - is class registered here?
//

HRESULT FileRegistry::IsClassRegistered
    (
    REFCLSID                clsid
    )
    {
    RegMap_t::const_iterator i = m_regmap.find (clsid);
    if (i == m_regmap.end ())
        {
        for (i = m_regmap.begin (); i != m_regmap.end (); ++i)
            cout << (*i).first << endl;
        return REGDB_E_CLASSNOTREG;
        }

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// FileRegistry::CreateInstance - create an instance of the given class
//

HRESULT FileRegistry::CreateInstance
    (
    REFCLSID                clsid,
    IUnknown *              pUnkOuter,
    DWORD                   dwClsContext,
    const char *            hint,
    ULONG                   cMQIs,
    MULTI_QI *              pMQIs
    )
    {
    IClassFactory* pCF = 0;

    // Get the class-factory...
    HRESULT hr = GetClassObject (clsid,
                                 IID_IClassFactory,
                                 dwClsContext,
                                 hint,
                                 (IUnknown**) &pCF);
    
    // Now get the instance...
    IUnknown *punk = 0;
    hr = pCF->CreateInstance (pUnkOuter, IID_IUnknown, (void**) &punk);
    if (FAILED (hr))
        return hr;

    // Finished with the factory...
    pCF->Release ();
    
    // Ask for all the interfaces...
    for (ULONG i = 0; i < cMQIs; ++i)
        {
        pMQIs[i].hr = punk->QueryInterface (*(pMQIs[i].pIID),
                                            (void**) &pMQIs[i].pItf);
        }
    
    // Release our original ref...
    punk->Release ();

    return hr;
    }

//////////////////////////////////////////////////////////////////////////
//
// FileRegistry::GetClassObject - get the class-factory object
//

HRESULT FileRegistry::GetClassObject
    (
    REFCLSID                clsid,
    REFIID                  iid,
    DWORD                   dwClsContext,
    const char *            hint,
    IUnknown **             ppClsObj
    )
    {
    // Find the reg-entry...
    RegMap_t::const_iterator i = m_regmap.find (clsid);
    if (i == m_regmap.end ())
        return REGDB_E_CLASSNOTREG;

    string fileName = (*i).second.library;

    // Open the shared-lib...
    void* handle = dlopen (fileName.c_str (), RTLD_NOW | RTLD_GLOBAL);
    if (! handle)
        {
        cout << dlerror () << endl;
        return E_UNEXPECTED;
        }
    
    // Find the symbol...
    PFN_GETCLASSOBJECT gco =
        (PFN_GETCLASSOBJECT) dlsym (handle, "DllGetClassObject");

    if (! gco)
        {
        dlclose (handle);
        cout << dlerror () << endl;
        return E_UNEXPECTED;
        }

    // Call the function to get the class-factory...
    return gco (clsid, iid, (void**) ppClsObj);
    }

//////////////////////////////////////////////////////////////////////////
//
// FileRegistry::GetClassID - enumerate class IDs
//

HRESULT FileRegistry::GetClassID
    (
    DWORD                   dwIndex,
    LPCLSID                 pclsid
    )
    {
    RegMap_t::const_iterator iter = m_regmap.begin ();
    for (DWORD i = 0; i < dwIndex; ++i)
        {
        ++iter;
        if (iter == m_regmap.end ())
            return E_FAIL;
        }

    *pclsid = (*iter).first;
        
    return S_OK;
    }

#endif  /* defined SOLARIS || LINUX */




