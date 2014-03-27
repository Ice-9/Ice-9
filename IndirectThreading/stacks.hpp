
# define    STACK_UP            1

template <int Size, class Type> class Stack {
    long long int guardA[4];
    Type array[Size];
    long long int guardZ[4];
    Type *sp;

public:
    Type *&ptr()                { return sp; }

# if STACK_UP
    void push(Type val)         { *++sp = val; }
    Type pop()                  { return *sp--; }     // umm, sucks for storing into objects...
    void inc(int n)             { sp += n; }
    void dec(int n)             { sp -= n; }
    void inc()                  { sp++; }
    void dec()                  { --sp; }

    Type & operator [] (int n)  { return sp[1 - n]; }

    bool overflow()             { return sp >= array + Size - 1; }
    bool underflow()            { return sp < array - 1; }

    void clear()                { sp = array - 1; bzero(guardA, sizeof(guardA)); bzero(guardZ, sizeof(guardZ)); }
    int depth()                 { return sp - array + 1; }
# else
    void push(Type val)         { *--sp = val; }
    Type pop()                  { return *sp++; }
    void inc(int n)             { sp -= n; }
    void dec(int n)             { sp += n; }
    void inc()                  { sp--; }
    void dec()                  { ++sp; }

    Type & operator [] (int n)  { return sp[n - 1]; }

    bool overflow()             { return sp <= array; }
    bool underflow()            { return sp > array + Size; }

    void clear()                { sp = array + Size; bzero(guardA, sizeof(guardA)); bzero(guardZ, sizeof(guardZ)); }
    int depth()                 { return array + Size - sp; }
# endif
};


template <int Size, class Type, class Type2> class Stack2 {
    long long int guardA[4];
    Type array[Size];
    long long int guardZ[4];
    Type *sp;

public:
    Type *&ptr()                { return sp; }

# if STACK_UP
    void push(Type val)         { *++sp = val; }
    void push2(Type2 val)       { *(Type2 *) (sp + 1) = val; sp += 2; }
    void bpPush(BytePtr val)    { *++sp = Type(val); }
    void cpPush(CharPtr val)    { *++sp = Type(val); }
    void clpPush(CellPtr val)   { *++sp = Type(val); }
    void clppPush(CellPtr *val) { *++sp = Type(val); }
    Type pop()                  { return *sp--; }
    Type2 pop2()                { sp -= 2; return *(Type2 *) (sp + 1); }
    BytePtr bpPop()             { return BytePtr(*sp--); }
    CharPtr cpPop()             { return CharPtr(*sp--); }
    CellPtr clpPop()            { return CellPtr(*sp--); }
    CellPtr *clppPop()          { return (CellPtr *) (*sp--); }

    void inc(int n)             { sp += n; }
    void dec(int n)             { sp -= n; }
    void inc()                  { sp++; }
    void dec()                  { --sp; }

    Type  &operator[] (int n)   { return sp[1 - n]; }
    Type2 &operator() (int n)   { return *(Type2 *) (sp - n); }

    bool overflow()             { return sp >= array + Size - 1; }
    bool underflow()            { return sp < array - 1; }

    void clear()                { sp = array - 1; bzero(guardA, sizeof(guardA)); bzero(guardZ, sizeof(guardZ)); }
    int depth()                 { return sp - array + 1; }
# else
    void push(Type val)         { *--sp = val; }
    void push2(Type2 val)       { sp -= 2; *(Type2 *) sp = val; }
    void bpPush(BytePtr val)    { *--sp = Type(val); }
    void cpPush(CharPtr val)    { *--sp = Type(val); }
    void clpPush(CellPtr val)   { *--sp = Type(val); }
    void clppPush(CellPtr *val) { *--sp = Type(val); }
    Type pop()                  { return *sp++; }
    Type2 pop2()                { sp += 2; return *(Type2 *) (sp - 2); }
    CharPtr bpPop()             { return BytePtr(*sp++); }
    CharPtr cpPop()             { return CharPtr(*sp++); }
    CellPtr clpPop()            { return CellPtr(*sp++); }
    CellPtr *clppPop()          { return (CellPtr *) (*sp++); }

    void inc(int n)             { sp -= n; }
    void dec(int n)             { sp += n; }
    void inc()                  { sp--; }
    void dec()                  { ++sp; }

    Type  &operator[] (int n)   { return sp[n - 1]; }
    Type2 &operator() (int n)   { return *(Type2 *) (sp + n - 1); }

    bool overflow()             { return sp <= array; }
    bool underflow()            { return sp > array + Size; }

    void clear()                { sp = array + Size; bzero(guardA, sizeof(guardA)); bzero(guardZ, sizeof(guardZ)); }
    int depth()                 { return array + Size - sp; }
# endif
};

