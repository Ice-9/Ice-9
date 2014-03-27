
# include   "standard.hpp"
# include   "memory9.hpp"

namespace Ice9 {
    namespace Memory {

        void QualifierExtent::AllocateQe(Word nQualifiers, QualifierExtent::Ptr &qepList, QualPtr &qpFree) {
            QualifierExtent::Ptr qep = QualifierExtent::Ptr(malloc(sizeof(QualifierExtent) + nQualifiers * sizeof(Qualifier)));
            if(! qep)
                ABORT("Memory exhausted during Qualifier Extent allocation\n");

            qep->nq = nQualifiers;
            qep->qepNext = qepList;
            qepList = qep;

            Word i;
            for(i = 1; i < nQualifiers; ++i)
                qep->qArray[i - 1].qpNext = &qep->qArray[i];
            qep->qArray[nQualifiers - 1].qpNext = qpFree;
            qpFree = &qep->qArray[0];
        }   /* QualifierExtent::AllocateQe */

    };  /* namespace Ice9::Memory */
};  /* namespace Ice9 */

