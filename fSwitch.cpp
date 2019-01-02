#include "mdk/mdk.h"
#include "math.h"
#include <windows.h>

#pragma optimize ("awy", on) 

#define PROGRAM_NAME "fSwitch"
#ifdef _DEBUG
#define PROGRAM_VERSION "1.06.0 beta"
#else
#define PROGRAM_VERSION "1.06.0"
#endif
#define INPUTSEL_NONE 0xFF
#define INPUTSEL_DEFAULT 0x01
#define CROSSFADE_NONE 0xFFFF
#define CROSSFADE_DEFAULT 0x04C

float docrossfade (float f)
{
	float crossfade = (float)(exp((float)f/13665)*1000-1000.6);
	if (crossfade < 0)
		crossfade = 0;
	return crossfade;
}

CMachineParameter const paraInputSelect =
{
	pt_byte,                        // Parameter data type
	"Input",                        // Parameter name as its shown in the parameter
                                                // window
	"Input",            // Parameter description as its shown in
                                                //the pattern view's statusbar
	0,                                    // Minimum value
	0x11,                        // Maximum value
	INPUTSEL_NONE,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
	MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
	INPUTSEL_DEFAULT                        // the default slider value
};

CMachineParameter const paraCrossFade =
{
	pt_word,                        // Parameter data type
	"X-fade",                        // Parameter name as its shown in the parameter
                                                // window
	"Cross fade",            // Parameter description as its shown in
                                                //the pattern view's statusbar
	0,                                    // Minimum value
	0x8000,                        // Maximum value
	CROSSFADE_NONE,                        // Novalue, this value means "nothing
                                                // happened" in the mi::Tick procedure
	MPF_STATE,                        // Parameter options, MPF_STATE makes it 
                                                // appears as a slider
	CROSSFADE_DEFAULT                        // the default slider value
};



CMachineParameter const *pParameters[] = {
	&paraInputSelect,
	&paraCrossFade
};

CMachineAttribute const *pAttributes[] = { NULL };


///////////////////////////////////////////////
// PART TWO
///////////////////////////////////////////////

#pragma pack(1)                        

class gvals
{
public:
	byte inputsel;
	word crossfade;
};

#pragma pack()

CMachineInfo const MacInfo = 
{
	MT_EFFECT,                        // Machine type
	MI_VERSION,                        // Machine interface version
	MIF_DOES_INPUT_MIXING,            // Machine flags
	0,                                    // min tracks
	0,                                    // max tracks
	2,                                    // numGlobalParameters
	0,                                    // numTrackParameters
	pParameters,            // pointer to parameter stuff
	0,                                    // numAttributes
	pAttributes,            // pointer to attribute stuff
#ifdef _DEBUG
	"Nafai's fSwitch (debug)",            // Full Machine Name
#else
	"Nafai's fSwitch",            // Full Machine Name
#endif
	"fSwitch",                        // Short name
	"Thor Asmund",            // Author name
	"&About..."                        // Right click menu commands
};


class miex;

class mi;

class miex : public CMDKMachineInterfaceEx
{
public:
	virtual void Input(float *psamples, int numsamples, float amp); 
	virtual bool HandleInput(int index, int amp, int pan); //{ return false; }
	virtual void SetInputChannels(char const *macname, bool stereo); //{}
/*
	virtual void AddInput(char const *macname, bool stereo);            // called when input is added to a machine
	virtual void DeleteInput(char const *macename);                                    
	virtual void RenameInput(char const *macoldname, char const *macnewname); 
*/
public:
	mi *pmi;
};


class mi : public CMDKMachineInterface
{
public:
	mi();
	virtual ~mi();
	virtual void Tick();
	virtual void MDKInit(CMachineDataInput * const pi);
	virtual bool MDKWork(float *psamples, int numsamples, int const mode);
	virtual bool MDKWorkStereo(float *psamples, int numsamples, int const mode);
	virtual void Command(int const i);
	virtual void MDKSave(CMachineDataOutput * const po);
	virtual char const *DescribeValue(int const param, int const value);
	virtual CMDKMachineInterfaceEx *GetEx() { return &ex; }
	virtual void OutputModeChanged(bool stereo) {}

public:
	miex ex;

public:
	int arraysize,lastbufsize;
	float *thearray;
	float *fadearray;
	byte previnputsel,inputsel;
	float crossfade;
	int curinput;
	int fadeval, maxfadeval;

	gvals gval;
};



///////////////////////////////////////////////
// PART THREE
///////////////////////////////////////////////

// Miex
/*
void miex::AddInput(char const *macname, bool stereo) { }
void miex::DeleteInput(char const *macename) { }
void miex::RenameInput(char const *macoldname, char const *macnewname) { }
	*/
void miex::SetInputChannels(char const *macname, bool stereo) { }
bool miex::HandleInput(int index, int amp, int pan) { return false; }
void miex::Input(float *psamples, int numsamples, float amp)
{
	float *restore = psamples;

	if (pmi->fadeval == 0 && pmi->inputsel == 0)
		return;

	pmi->lastbufsize = numsamples;
	if (numsamples > pmi->arraysize)
	{
		if (pmi->thearray)
			delete pmi->thearray;
		if (pmi->fadearray)
			delete pmi->fadearray;
		pmi->arraysize = numsamples;
		pmi->thearray = new float[2*pmi->arraysize];
		pmi->fadearray = new float[2*pmi->arraysize];
		for (int i = 0; i < numsamples*2; i++) {
			pmi->thearray[i] = 0.0f;
			pmi->fadearray[i] = 0.0f;
		}
	}

	int i;
	if (pmi->inputsel == 17) {
		for (i = 0; i < numsamples*2; i++) 
			pmi->thearray[i] = pmi->thearray[i] + amp*(*psamples++);
	} 
	else if (pmi->curinput == pmi->inputsel-1)
	{
		for (i = 0; i < numsamples*2; i++) 
			pmi->thearray[i] = amp*(*psamples++);
	}

	// Store fade from values
	if (pmi->fadeval > 0 && pmi->curinput == pmi->previnputsel-1)
	{
		psamples = restore;
		for (i = 0; i < numsamples*2; i++) 
			pmi->fadearray[i] = amp*(*psamples++);
	}
	pmi->curinput++;
}


// Mi
mi::mi() 
{
	GlobalVals = &gval;
	thearray = NULL;
	fadearray = NULL;
}

mi::~mi()
{
	if (thearray)
		delete thearray;
	if (fadearray)
		delete fadearray;
}

void mi::MDKInit(CMachineDataInput * const pi)
{
	SetOutputMode( true ); // No mono sounds
	ex.pmi = this;
	inputsel = INPUTSEL_DEFAULT;
	previnputsel = INPUTSEL_DEFAULT;
	crossfade = docrossfade(CROSSFADE_DEFAULT);
	arraysize = 1024;
	thearray = new float[2*arraysize];
	fadearray = new float[2*arraysize];
	fadeval = 0;
	maxfadeval = 0;
}

void mi::MDKSave(CMachineDataOutput * const po) { }

void mi::Tick() {
	if (gval.inputsel != INPUTSEL_NONE)
	{
		previnputsel = inputsel;
		inputsel = gval.inputsel;
		int storemaxfadeval = maxfadeval;
		maxfadeval = (int)(crossfade/1000 * pMasterInfo->SamplesPerSec);
		if (storemaxfadeval == 0)
			fadeval = maxfadeval;
		else
			fadeval = (int) (((float)storemaxfadeval-fadeval)/storemaxfadeval*maxfadeval);
	}
	if (gval.crossfade != CROSSFADE_NONE)
	{
		crossfade = docrossfade(gval.crossfade);
	}
}

bool mi::MDKWork(float *psamples, int numsamples, int const mode)
{
	return false;
}

bool mi::MDKWorkStereo(float *psamples, int numsamples, int const mode)
{
	if (mode==WM_WRITE)
		return false;
	if (mode==WM_NOIO)
		return false;
	if (mode==WM_READ)                        // <thru>
		return false;

	if (fadeval == 0 && inputsel == 0)
		return false;

	for (int i = 0; i < numsamples; i++)
	{
		if (fadeval == 0) {
			*psamples++ = thearray[i*2];
			*psamples++ = thearray[i*2+1];
		}
		else 
		{
			*psamples++ = thearray[i*2]*(maxfadeval-fadeval)/maxfadeval + fadearray[i*2]*fadeval/maxfadeval;
			*psamples++ = thearray[i*2+1]*(maxfadeval-fadeval)/maxfadeval + fadearray[i*2+1]*fadeval/maxfadeval;
			fadeval--;
		}
		thearray[i*2] = 0.0f;
		thearray[i*2+1] = 0.0f;
		fadearray[i*2] = 0.0f;
		fadearray[i*2+1] = 0.0f;
	}

	curinput = 0;
	return true;
}

void mi::Command(int const i)
{
	char txt[512];
	switch (i)
	{
	case 0:
		sprintf(txt,"%s v%s\n(c) Thor Asmund 2004\n\nA simple effect to switch between multiple inputs",PROGRAM_NAME,PROGRAM_VERSION);
		MessageBox(NULL,txt,"About fSwitch",MB_OK|MB_SYSTEMMODAL);
		break;
	default:
		break;
	}
}
char const *mi::DescribeValue(int const param, int const value)
{
	static char txt[16];
	switch(param)
	{
	case 0:
		if (value == 0)
			strcpy(txt,"None");
		else if (value == 17)
			strcpy(txt,"All");
		else
			sprintf(txt,"%d", value );
		return txt;
		break;
	case 1:
		if (value == 0)
			strcpy(txt,"Off");
		else
			sprintf(txt,"%.1f ms", docrossfade((float)value));
		return txt;
		break;
	default:
		return NULL;
	}
}

#pragma optimize ("", on) 

DLL_EXPORTS
