#include <iostream>
#include <cstdint>

// ------------------------------------------------------ //

#define STACK_MAXDEPTH 16
#define VARIABLES_MAXCOUNT 64

class CScript;

// ------------------------------------------------------ //

typedef uint32_t enginems_t;
typedef void (*ScriptFn)(CScript*, uint16_t);
enum eScriptState : uint8_t
{
    SCRSTATE_INACTIVE = 0,
    SCRSTATE_ACTIVE,
    
    SCRIPT_MAXSTATES
};
enum eLogicState : uint8_t
{
    LOGSTATE_ZERO = 0,
    
    LOGIC_MAXSTATES
};
enum eVariableType : uint8_t
{
    VARTYPE_NONE = 0,
    VARTYPE_INT,
    VARTYPE_FLOAT,
    VARTYPE_STRING,
    VARTYPE_STRINGPTR,
    VARTYPE_PTR,
    
    VARIABLE_MAXTYPES
};

// ------------------------------------------------------ //

struct ScriptVariable
{
    union
    {
        bool b;
        short w;
        int32_t i;
        uint32_t u;
        float f;
        void* p;
        char* c;
    };
    eVariableType type;
};
extern ScriptVariable ScriptParams[VARIABLES_MAXCOUNT];

// ------------------------------------------------------ //

class CScript
{
public:
    CScript(const char* name, CScript* parent)
    {
        if(parent)
        {
            next = parent->next;
            prev = parent;
            
            parent->next = this;
            if(next) next->prev = this;
        }
        else
        {
            prev = NULL;
            next = NULL;
        }
        starttime = 0;
        state = eScriptState::SCRSTATE_INACTIVE;
        logic = eLogicState::LOGSTATE_ZERO;
        stackDepth = 0;
    }
    inline bool PushStack()
    {
        if(stackDepth >= STACK_MAXDEPTH) return false;
        stack[stackDepth++] = scriptPointer;
        return true;
    }
    inline bool PopStack()
    {
        if(stackDepth <= 0) return false;
        scriptPointer = stack[--stackDepth];
        return true;
    }
    inline uint8_t* JumpPtr(int32_t offset)
    {
        if(offset < 0)
        {
            return scriptBase - offset;
        }
        else
        {
            return scriptBase + offset;
        }
    }
    inline void Jump(int32_t offset)
    {
        scriptPointer = JumpPtr(offset);
    }
    inline void Jump(uint8_t* pointer)
    {
        scriptPointer = pointer;
    }
    inline uint8_t GetByte(bool keep = false)
    {
        uint8_t val = *scriptPointer;
        if(!keep) scriptPointer += 1;
        return val;
    }
    inline uint16_t GetWide(bool keep = false)
    {
        uint16_t val = *(uint16_t*)scriptPointer;
        if(!keep) scriptPointer += 2;
        return val;
    }
    inline int32_t GetInt(bool keep = false)
    {
        int32_t val = *(int32_t*)scriptPointer;
        if(!keep) scriptPointer += 4;
        return val;
    }
    inline uint32_t GetUInt(bool keep = false)
    {
        uint32_t val = *(uint32_t*)scriptPointer;
        if(!keep) scriptPointer += 4;
        return val;
    }
    inline uint64_t GetLong(bool keep = false)
    {
        uint64_t val = *(uint64_t*)scriptPointer;
        if(!keep) scriptPointer += 8;
        return val;
    }
    inline void* GetPtr(bool keep = false) // stupid :P
    {
        void* val = *(void**)scriptPointer;
        if(!keep) scriptPointer += sizeof(void*);
        return val;
    }
    inline char* GetString(bool keep = false)
    {
        uint8_t type = GetByte(true);
        if(type == eVariableType::VARTYPE_STRING)
        {
            GetByte(keep);
            int offset = GetInt(keep);
            return (char*)(JumpPtr(offset));
        }
        else if(type == eVariableType::VARTYPE_STRINGPTR ||
                type == eVariableType::VARTYPE_PTR)
        {
            GetByte(keep);
            return (char*)(GetPtr(keep));
        }
        return NULL;
    }
    inline void CollectParams(uint8_t count = 1)
    {
        for(uint8_t i = 0; i < count && i < VARIABLES_MAXCOUNT; ++i)
        {
            ScriptParams[i].type = (eVariableType)GetByte(true);
            char* t = GetString(); // +8 on 64bit!
            if(t) ScriptParams[i].c = t;
            else
            {
                GetByte();
                ScriptParams[i].i = GetInt();
            }
        }
    }
    
public:
    CScript* prev;
    CScript* next;
    char scriptname[24];
    enginems_t starttime;
    eScriptState state;
    eLogicState logic;
    uint8_t flag;
    uint8_t stackDepth;
    uint32_t scriptLength;
    uint8_t* scriptBase;
    uint8_t* scriptPointer;
    uint8_t* stack[STACK_MAXDEPTH];
    ScriptVariable vars[VARIABLES_MAXCOUNT];
};

// ------------------------------------------------------ //

class CScriptFunction
{
public:
    

public:
    int args;
};

// ------------------------------------------------------ //

class CScripts
{
public:
    CScripts()
    {
        
    }
    inline static void Init()
    {
        CScripts::pOpcodes[0xFFFF] = [](CScript* script, uint16_t opcode)
        {
            CScripts::DeactivateScript(script);
        };
    }
    inline static void Update()
    {
        CScript* script = CScripts::pActiveScripts;
        while(script)
        {
            CScripts::ProcessOneCommand(script);
            script = script->next;
        }
    }
    inline static void DeactivateScript(CScript* script)
    {
        if(script->state == eScriptState::SCRSTATE_INACTIVE) return;
        if(script->state == eScriptState::SCRSTATE_ACTIVE)
        {
            if(CScripts::pActiveScripts == script)
            {
                if(script->next != NULL) CScripts::pActiveScripts = script->next;
                else CScripts::pActiveScripts = NULL;
            }
            else
            {
                if(script->next) script->next->prev = script->prev;
                if(script->prev) script->prev->next = script->next;
            }
        }
        
        script->prev = NULL;
        if(CScripts::pInactiveScripts == NULL)
        {
            script->next = NULL;
            CScripts::pInactiveScripts = script;
        }
        else
        {
            script->next = CScripts::pInactiveScripts;
            CScripts::pInactiveScripts = script;
        }
        script->state = eScriptState::SCRSTATE_INACTIVE;
    }
    inline static void ActivateScript(CScript* script)
    {
        if(script->state == eScriptState::SCRSTATE_ACTIVE) return;
        if(script->state == eScriptState::SCRSTATE_INACTIVE)
        {
            if(CScripts::pInactiveScripts == script)
            {
                if(script->next != NULL) CScripts::pInactiveScripts = script->next;
                else CScripts::pInactiveScripts = NULL;
            }
            else
            {
                if(script->next) script->next->prev = script->prev;
                if(script->prev) script->prev->next = script->next;
            }
        }
        
        script->prev = NULL;
        if(CScripts::pActiveScripts == NULL)
        {
            script->next = NULL;
            CScripts::pActiveScripts = script;
        }
        else
        {
            script->next = CScripts::pActiveScripts;
            CScripts::pActiveScripts = script;
        }
        script->state = eScriptState::SCRSTATE_ACTIVE;
    }
    inline static void ProcessOneCommand(CScript* script)
    {
        uint16_t opcode = script->GetWide();
        if(opcode) pOpcodes[opcode](script, opcode);
        script->GetByte(false); // we have ZERO byte (means END OF OPCODE)
    }
    inline static CScript* NewScript(const char* name, const char* scriptdata, uint32_t scriptlen = 0, bool instantiate = true)
    {
        CScript* script = new CScript(name, NULL);
        script->scriptLength = scriptlen == 0 ? strlen(scriptdata) : scriptlen;
        if(instantiate) // a bit safer..?
        {
            script->scriptBase = new uint8_t[script->scriptLength + 3];
            memcpy(script->scriptBase, scriptdata, script->scriptLength);
            
            script->scriptBase[script->scriptLength + 0] = 0xFF;
            script->scriptBase[script->scriptLength + 1] = 0xFF;
            script->scriptBase[script->scriptLength + 2] = 0x00;
        }
        else
        {
            script->scriptBase = (uint8_t*)scriptdata;
        }
        script->scriptPointer = script->scriptBase;
        return script;
    }
    
public:
    static CScript* pActiveScripts;
    static CScript* pInactiveScripts;
    
    static ScriptFn pOpcodes[0xFFFF];
};

// ------------------------------------------------------ //
