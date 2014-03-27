
# include   "standard.hpp"
# include   "memory9.hpp"


// namespace IM { using namespace Ice9::Memory; }
using namespace Ice9::Memory;

int AnyKey(const char *cp) {
    int ch;

    fflush(stderr);
    printf(cp ? cp : "Press the ANY key to continue: ");
    for(;;) {
        ch = getchar();
        if(ch == '\n') break;
        if(ch == 'q') exit(1);
        if(ch == 'Q') exit(1);

        if(ch == EOF) {
            printf("\n");
            exit(1);
        }
    }

    return ch;
}


# include <signal.h>

void Catcher(int sig) {
    // TODO: capable of deadlocking if the signal comes in a STDIO critical?

    static char mesg[100];
    sprintf(mesg, "\nCAUGHT %d: %s\n", sig, strsignal(sig));
    write(2, mesg, strlen(mesg));
    exit(1);
}


int main(int argc, char *argv[]) {
    char buf[200];
    OP opRoot, opX, opA, opB;
    OP op0p2, op0p1, op0, op0m1, op0m2;
    OP op1, op2, op3;
    OP qp1, qp2, qp3;

    struct sigaction act;
    act.sa_handler = Catcher;
    sigemptyset(&act.sa_mask);    // TODO: mask out everything??
    act.sa_flags = 0;

    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGILL, &act, NULL);
    sigaction(SIGABRT, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGSYS, &act, NULL);

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    Allocator::Init();

    // CHATTER("\n");
    // CHATTER("ALIGN SIZE: %2d (%d words)\n", ALIGN_SIZE_BYTES,  ALIGN_SIZE_BYTES  / sizeof(Word));
    // CHATTER("NODE SIZE : %2d (%d words)\n", sizeof(Node),      sizeof(Node)      / sizeof(Word));
    // CHATTER("QUAL SIZE : %2d (%d words)\n", sizeof(Qualifier), sizeof(Qualifier) / sizeof(Word));
    // CHATTER("NHDR SIZE : %2d (%d words)\n", sizeof(NodeHdr),   sizeof(NodeHdr)   / sizeof(Word));
    // CHATTER("__align_u : %2d (%d words)\n", sizeof(__align_u), sizeof(__align_u) / sizeof(Word));
    // CHATTER("\n");
    CHATTY(Allocator::CheckArena("UNUSED ARENA"));

    Allocator::MakeSpaces();
    OP::StandardClasses();
    OP::StandardObjects();

    opRoot = OP::opObjectNew(OP::ObjectClass, 0, 0, 5);
    opRoot.registerRoot();

#define SIZED 256

    op0p2 = OP::opObjectNew(OP::ObjectClass, 0, 0, SIZED + 2);
    op0p1 = OP::opObjectNew(OP::ObjectClass, 0, 0, SIZED + 1);
    op0   = OP::opObjectNew(OP::ObjectClass, 0, 0, SIZED);
    op0m1 = OP::opObjectNew(OP::ObjectClass, 0, 0, SIZED - 1);
    op0m2 = OP::opObjectNew(OP::ObjectClass, 0, 0, SIZED - 2);
    AnyKey();
    /// exit(0);

    opX = OP::opObjectNew(OP::ObjectClass, 0, 0, 1);
    opB = OP::opObjectNew(OP::ObjectClass, 1, 1, 20);
    opA = OP::opArrayNew(OP::ObjectClass, 1, 1, 1, 0, 19);
    for(int i = 0; i < 20; ++i) {
        opB.ByteAt(i) = i + 1;
        opA.ByteAt(i) = i + 1;
    }
    // CHATTY(opB.dump("opB"));
    // CHATTY(opA.dump("opA"));
    // exit(0);
    // AnyKey();

    opRoot.opSetAt(1, op0);
    op0.opSetAt(SIZED - 1, opX);
    op0.opSetAt(SIZED - 2, opB);
    op0.opSetAt(SIZED - 3, opA);
    op0.opSetAt(SIZED - 4, op0p2);
    op0.opSetAt(SIZED - 5, op0p1);
    op0.opSetAt(SIZED - 6, op0m1);
    op0.opSetAt(SIZED - 7, op0m2);

    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));
    Allocator::ReclaimInaccessibleObjects();
    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));

    op1 = OP::opObjectNew(OP::ObjectClass, 0, 0, 2);
    op2 = OP::opObjectNew(OP::ObjectClass, 0, 0, 3);
    op3 = OP::opObjectNew(OP::ObjectClass, 0, 0, 4);

    qp1 = OP::opQualStrRef(10, "abcdefghij");
    qp2 = OP::opQualStrCpy(10, "abcdefghij");
    qp3 = OP::opQualSub( 3, qp2, 3);

    op1.opSetAt(1, op0); CHATTER("*** UPDATE op1[1] = op0 *%tX\n", op0);
    op2.opSetAt(1, op0); CHATTER("*** UPDATE op2[1] = op0 *%tX\n", op0);
    op3.opSetAt(1, op0); CHATTER("*** UPDATE op3[1] = op0 *%tX\n", op0);

    op0.opSetAt(1, op0); CHATTER("*** UPDATE op0[1] = op0 *%tX\n", op0);
    op0.opSetAt(2, op1); CHATTER("*** UPDATE op0[2] = op1 *%tX\n", op1);
    op0.opSetAt(3, op2); CHATTER("*** UPDATE op0[3] = op2 *%tX\n", op2);
    op0.opSetAt(4, op3); CHATTER("*** UPDATE op0[4] = op3 *%tX\n", op3);

    op0.opSetAt(5, qp1); CHATTER("*** UPDATE op0[5] = qp1 *%tX\n", qp1);
    op0.opSetAt(6, qp2); CHATTER("*** UPDATE op0[6] = qp2 *%tX\n", qp2);
    op0.opSetAt(7, qp3); CHATTER("*** UPDATE op0[7] = qp3 *%tX\n", qp3);
    CHATTER("\n");

    CHATTY(Allocator::CheckArena("All objects tangled up\n"));
    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));
    CHATTY(op1.dump("op1"));
    CHATTY(op2.dump("op2"));
    CHATTY(op3.dump("op3"));
    CHATTY(qp1.dump("qp1"));
    CHATTY(qp2.dump("qp2"));
    CHATTY(qp3.dump("qp3"));
    CHATTER("\n");

    Allocator::ReclaimInaccessibleObjects();
    CHATTY(Allocator::CheckArena("AFTER ReclaimInaccessibleObjects()\n"));
    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));
    CHATTY(op1.dump("op1"));
    CHATTY(op2.dump("op2"));
    CHATTY(op3.dump("op3"));
    CHATTY(qp1.dump("qp1"));
    CHATTY(qp2.dump("qp2"));
    CHATTY(qp3.dump("qp3"));

    assert(OP::ObjectClass.classOf() == OP::ClassClass);
    assert(opRoot.classOf() == OP::ObjectClass);
    assert(opX.classOf() == OP::ObjectClass);
    assert(opA.classOf() == OP::ObjectClass);
    assert(opB.classOf() == OP::ObjectClass);
    assert(op0p2.classOf() == OP::ObjectClass);
    assert(op0p1.classOf() == OP::ObjectClass);
    assert(op0.classOf() == OP::ObjectClass);
    assert(op0m1.classOf() == OP::ObjectClass);
    assert(op0m2.classOf() == OP::ObjectClass);
    assert(op1.classOf() == OP::ObjectClass);
    assert(op2.classOf() == OP::ObjectClass);
    assert(op3.classOf() == OP::ObjectClass);
    assert(qp1.classOf() == OP::QualClass);
    assert(qp2.classOf() == OP::QualClass);
    assert(qp3.classOf() == OP::QualClass);

    op0.opSetAt(2, NIL); CHATTER("*** NILUP op0[2] = NIL (op1 *%tX)\n", op1);
    op0.opSetAt(3, NIL); CHATTER("*** NILUP op0[3] = NIL (op2 *%tX)\n", op2);
    op0.opSetAt(4, NIL); CHATTER("*** NILUP op0[4] = NIL (op3 *%tX)\n", op3);
    op0.opSetAt(5, NIL); CHATTER("*** NILUP op0[5] = NIL (qp1 *%tX)\n", qp1);
    op0.opSetAt(6, NIL); CHATTER("*** NILUP op0[6] = NIL (qp2 *%tX)\n", qp2);
    op0.opSetAt(7, NIL); CHATTER("*** NILUP op0[7] = NIL (qp3 *%tX)\n", qp3);
    CHATTER("\n");

    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));

    CHATTY(Allocator::CheckArena("AFTER NILUP"));
    Allocator::ReclaimInaccessibleObjects();
    CHATTY(Allocator::CheckArena("reclaimed after finish"));

    CHATTY(OP::ObjectClass.dump("opC"));
    CHATTY(opRoot.dump("opR"));
    CHATTY(op0.dump("op0"));
    CHATTY(opX.dump("opX"));
    CHATTY(opB.dump("opB"));
    CHATTY(opA.dump("opA"));

    opRoot.deregisterRoot();
    Allocator::ReclaimInaccessibleObjects();
    CHATTY(Allocator::CheckArena("reclaimed after deregistering root"));

    Allocator::Destroy();
}

