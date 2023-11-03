#include "jscript.h"

CScript* CScripts::pActiveScripts = NULL;
CScript* CScripts::pInactiveScripts = NULL;
ScriptFn CScripts::pOpcodes[] = {0};
ScriptVariable ScriptParams[VARIABLES_MAXCOUNT];

// BELOW IS AN EXAMPLE

void cul(CScript* script, uint16_t opcode)
{
    std::cout << "Hello World!" << std::endl;
}
void cul2(CScript* script, uint16_t opcode)
{
    std::cout << "Hello Worldddd!" << std::endl;
    script->CollectParams(1);
    std::cout << "Collected parameter is: " << ScriptParams[0].c << std::endl;
}

const char* scrData = "\x22\x22\x00\x21\x22\x03\x11\x00\x00\x00\x00"
            "\x91\x22\x00\xFF\xFF\x00";

int main()
{
    CScripts::Init();
    CScripts::pOpcodes[0x2222] = cul;
    CScripts::pOpcodes[0x2221] = cul2;
    CScripts::pOpcodes[0x2291] = [](CScript*,uint16_t){endit = false;};
    
    CScript* scr = CScripts::NewScript("", scrData, 0, false);
    CScripts::ActivateScript(scr);
	
	// game loop
	while(true) CScripts::Update();
	std::cout << "End of the loop";
}