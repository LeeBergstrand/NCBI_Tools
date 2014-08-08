/*  $Id: cn3d_threader.cpp 199888 2010-08-03 18:29:53Z thiessen $
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
* Authors:  Paul Thiessen
*
* File Description:
*       class to isolate and run the threader
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp> // must come first to avoid NCBI type clashes
#include <corelib/ncbistl.hpp>

#include <memory>

#include <objects/scoremat/scoremat__.hpp>

#include <algo/structure/threader/thrdatd.h>
#include <algo/structure/threader/thrddecl.h>

#include "remove_header_conflicts.hpp"

#include "block_multiple_alignment.hpp"
#include "cn3d_threader.hpp"
#include "sequence_set.hpp"
#include "molecule.hpp"
#include "structure_set.hpp"
#include "residue.hpp"
#include "coord_set.hpp"
#include "atom_set.hpp"
#include "cn3d_tools.hpp"
#include "molecule_identifier.hpp"
#include "sequence_viewer.hpp"
#include "cn3d_pssm.hpp"

USING_NCBI_SCOPE;
USING_SCOPE(objects);


BEGIN_SCOPE(Cn3D)

// define to include debugging output (threader structures to files)
//#define DEBUG_THREADER

// always do debugging in Debug build mode
#if (!defined(DEBUG_THREADER) && defined(_DEBUG))
#define DEBUG_THREADER
#endif

// default threading options
ThreaderOptions::ThreaderOptions(void) :
    mergeAfterEachSequence(true),
    freezeIsolatedBlocks(true),
    weightPSSM(0.5),
    loopLengthMultiplier(1.5),
    nRandomStarts(1),
    nResultAlignments(1),
    terminalResidueCutoff(-1)
{
}

ThreaderOptions globalThreaderOptions;

// gives threader residue number for a character (-1 if non-standard aa)
static int LookupThreaderResidueNumberFromCharacterAbbrev(char r)
{
    typedef map < char, int > Char2Int;
    static Char2Int charMap;

    if (charMap.size() == 0) {
        for (unsigned int i=0; i<Threader::ThreaderResidues.size(); ++i)
            charMap[Threader::ThreaderResidues[i]] = i;
    }

    Char2Int::const_iterator c = charMap.find((unsigned char) toupper(r));
    return ((c != charMap.end()) ? c->second : -1);
}

const unsigned int Threader::SCALING_FACTOR = 1000000;

const string Threader::ThreaderResidues = "ARNDCQEGHILKMFPSTWYV";

// gives NCBIStdaa residue number for a threader residue number (or # for 'X' if char == -1)
unsigned int LookupNCBIStdaaNumberFromThreaderResidueNumber(char r)
{
    r = toupper((unsigned char) r);
    return LookupNCBIStdaaNumberFromCharacter(
            ((int)r >= 0 && r < (int)Threader::ThreaderResidues.size()) ? Threader::ThreaderResidues[r] : 'X');
}

Threader::Threader(AlignmentManager *parentAlnMgr) :
    alignmentManager(parentAlnMgr)
{
}

Threader::~Threader(void)
{
    ContactMap::iterator c, ce = contacts.end();
    for (c=contacts.begin(); c!=ce; ++c) FreeFldMtf(c->second);
}

Seq_Mtf * Threader::CreateSeqMtf(const BlockMultipleAlignment *multiple, double weightPSSM)
{
    // special case for "PSSM" of single-row "alignment" - just use BLOSUM62 score
    if (multiple->NRows() == 1) {
        Seq_Mtf *seqMtf = NewSeqMtf(multiple->GetMaster()->Length(), ThreaderResidues.size());
        for (unsigned int res=0; res<multiple->GetMaster()->Length(); ++res)
            for (unsigned int aa=0; aa<ThreaderResidues.size(); ++aa)
                seqMtf->ww[res][aa] = ThrdRound(
                    weightPSSM * SCALING_FACTOR *
                        GetBLOSUM62Score(multiple->GetMaster()->sequenceString[res], ThreaderResidues[aa]));
        TRACEMSG("Created Seq_Mtf (PSSM) from BLOSUM62 scores");
        return seqMtf;
    }

    // can't calculate PSSM with no blocks
    if (multiple->HasNoAlignedBlocks()) {
        ERRORMSG("Can't create Seq_Mtf with no aligned blocks");
        return NULL;
    }

    // convert matrix
    TRACEMSG("converting PSSM scores to Seq_Mtf");
    Seq_Mtf *seqMtf = NewSeqMtf(multiple->GetMaster()->Length(), ThreaderResidues.size());
    for (unsigned int c=0; c<multiple->GetMaster()->Length(); ++c)
        for (unsigned int r=0; r<ThreaderResidues.size(); ++r)
            seqMtf->ww[c][r] = ThrdRound(weightPSSM * SCALING_FACTOR *
                multiple->GetPSSM().GetPSSMScore(LookupNCBIStdaaNumberFromCharacter(ThreaderResidues[r]), c));

    return seqMtf;
}

Cor_Def * Threader::CreateCorDef(const BlockMultipleAlignment *multiple, double loopLengthMultiplier)
{
    static const unsigned int MIN_LOOP_MAX = 2;
    static const unsigned int EXTENSION_MAX = 10;

    BlockMultipleAlignment::UngappedAlignedBlockList alignedBlocks;
    multiple->GetUngappedAlignedBlocks(&alignedBlocks);
    Cor_Def *corDef = NewCorDef(alignedBlocks.size());

    // zero loop constraints for tails
    corDef->lll.llmn[0] = corDef->lll.llmn[alignedBlocks.size()] =
    corDef->lll.llmx[0] = corDef->lll.llmx[alignedBlocks.size()] =
    corDef->lll.lrfs[0] = corDef->lll.lrfs[alignedBlocks.size()] = 0;

    // loop constraints for unaligned regions between aligned blocks
    BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator
        b = alignedBlocks.begin(), be = alignedBlocks.end();
    unsigned int n, max;
    for (n=1, ++b; b!=be; ++n, ++b) {
        const UnalignedBlock *uaBlock = multiple->GetUnalignedBlockBefore(*b);
        if (uaBlock) {
            max = (unsigned int) (loopLengthMultiplier * uaBlock->width);
            if (max < MIN_LOOP_MAX) max = MIN_LOOP_MAX;
        } else
            max = MIN_LOOP_MAX;
        corDef->lll.llmn[n] = 0;
        corDef->lll.llmx[n] = max;
        corDef->lll.lrfs[n] = max;
    }

    // minimum block sizes (in coordinates of master)
    const Block::Range *range;
    int mid;
    for (n=0, b=alignedBlocks.begin(); b!=be; ++b, ++n) {
        range = (*b)->GetRangeOfRow(0);
        mid = (range->from + range->to) / 2;
        corDef->sll.rfpt[n] = mid;
        corDef->sll.nomn[n] = mid - range->from;
        corDef->sll.comn[n] = range->to - mid;
    }

    // left extension - trim to available residues
    corDef->sll.nomx[0] = corDef->sll.nomn[0] + EXTENSION_MAX;
    if (corDef->sll.rfpt[0] - corDef->sll.nomx[0] < 0)
        corDef->sll.nomx[0] = corDef->sll.rfpt[0];

    // right extension - trim to available residues
    corDef->sll.comx[alignedBlocks.size() - 1] = corDef->sll.comn[alignedBlocks.size() - 1] + EXTENSION_MAX;
    if (corDef->sll.rfpt[alignedBlocks.size() - 1] + corDef->sll.comx[alignedBlocks.size() - 1] >=
            (int)multiple->GetMaster()->Length())
        corDef->sll.comx[alignedBlocks.size() - 1] =
            multiple->GetMaster()->Length() - corDef->sll.rfpt[alignedBlocks.size() - 1] - 1;

    // extensions into unaligned areas between blocks
    const Block::Range *prevRange = NULL;
    unsigned int nUnaligned, extN;
    for (n=0, b=alignedBlocks.begin(); b!=be; ++b, ++n) {
        range = (*b)->GetRangeOfRow(0);
        if (n > 0) {
            nUnaligned = range->from - prevRange->to - 1;
            extN = nUnaligned / 2;  // N extension of right block gets smaller portion if nUnaligned is odd
            corDef->sll.nomx[n] = corDef->sll.nomn[n] + extN;
            corDef->sll.comx[n - 1] = corDef->sll.comn[n - 1] + nUnaligned - extN;
        }
        prevRange = range;
    }

    // no fixed segments
    corDef->fll.n = 0;

    return corDef;
}

Qry_Seq * Threader::CreateQrySeq(const BlockMultipleAlignment *multiple,
        const BlockMultipleAlignment *pairwise, int terminalCutoff)
{
    const Sequence *dependentSeq = pairwise->GetSequenceOfRow(1);
    BlockMultipleAlignment::UngappedAlignedBlockList multipleABlocks, pairwiseABlocks;
    multiple->GetUngappedAlignedBlocks(&multipleABlocks);
    pairwise->GetUngappedAlignedBlocks(&pairwiseABlocks);

    // query has # constraints = # blocks in multiple alignment
    Qry_Seq *qrySeq = NewQrySeq(dependentSeq->Length(), multipleABlocks.size());

    // fill in residue numbers
    unsigned int i;
    for (i=0; i<dependentSeq->Length(); ++i)
        qrySeq->sq[i] = LookupThreaderResidueNumberFromCharacterAbbrev(dependentSeq->sequenceString[i]);

    // if a block in the multiple is contained in the pairwise (looking at master coords),
    // then add a constraint to keep it there
    BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator
        m, me = multipleABlocks.end(), p, pe = pairwiseABlocks.end();
    const Block::Range *multipleRange, *pairwiseRange;
    for (i=0, m=multipleABlocks.begin(); m!=me; ++i, ++m) {
        multipleRange = (*m)->GetRangeOfRow(0);
        for (p=pairwiseABlocks.begin(); p!=pe; ++p) {
            pairwiseRange = (*p)->GetRangeOfRow(0);
            if (pairwiseRange->from <= multipleRange->from && pairwiseRange->to >= multipleRange->to) {
                int masterCenter = (multipleRange->from + multipleRange->to) / 2;
                // offset of master residue at center of multiple block
                int offset = masterCenter - pairwiseRange->from;
                pairwiseRange = (*p)->GetRangeOfRow(1);
                // dependent residue in pairwise aligned to master residue at center of multiple block
                qrySeq->sac.mn[i] = qrySeq->sac.mx[i] = pairwiseRange->from + offset;
                break;
            }
        }
    }

    // if a terminal block is unconstrained (mn,mx == -1), set limits for how far the new
    // (religned) block is allowed to be from the edge of the next block or from the
    // aligned region set upon demotion
    if (terminalCutoff >= 0) {
        if (qrySeq->sac.mn[0] == -1) {
            if (pairwise->alignDependentFrom >= 0) {
                qrySeq->sac.mn[0] = pairwise->alignDependentFrom - terminalCutoff;
            } else if (pairwiseABlocks.size() > 0) {
                const Block::Range *nextQryBlock = pairwiseABlocks.front()->GetRangeOfRow(1);
                qrySeq->sac.mn[0] = nextQryBlock->from - 1 - terminalCutoff;
            }
            if (qrySeq->sac.mn[0] < 0) qrySeq->sac.mn[0] = 0;
            INFOMSG("new N-terminal block constrained to query loc >= " << qrySeq->sac.mn[0] + 1);
        }
        if (qrySeq->sac.mx[multipleABlocks.size() - 1] == -1) {
            if (pairwise->alignDependentTo >= 0) {
                qrySeq->sac.mx[multipleABlocks.size() - 1] = pairwise->alignDependentTo + terminalCutoff;
            } else if (pairwiseABlocks.size() > 0) {
                const Block::Range *prevQryBlock = pairwiseABlocks.back()->GetRangeOfRow(1);
                qrySeq->sac.mx[multipleABlocks.size() - 1] = prevQryBlock->to + 1 + terminalCutoff;
            }
            if (qrySeq->sac.mx[multipleABlocks.size() - 1] >= qrySeq->n ||
                qrySeq->sac.mx[multipleABlocks.size() - 1] < 0)
                qrySeq->sac.mx[multipleABlocks.size() - 1] = qrySeq->n - 1;
            INFOMSG("new C-terminal block constrained to query loc <= "
                << qrySeq->sac.mx[multipleABlocks.size() - 1] + 1);
        }
    }

    return qrySeq;
}

/*----------------------------------------------------------------------------
 *  stuff to read in the contact potential. (code swiped from DDV)
 *---------------------------------------------------------------------------*/
static unsigned int CountWords(char* chs) {
    unsigned int     i, Count=0;
    bool  InsideStr = false;
    string Str(chs);
    for (i=0; i<Str.size(); ++i) {
        if (!InsideStr && (Str[i] != ' ')) {
            ++Count;
            InsideStr = true;
        }
        if (Str[i] == ' ') {
            InsideStr = false;
        }
    }
    return(Count);
}
static void ReadToRowOfEnergies(ifstream& InFile, unsigned int NumResTypes) {
    char  Str[1024];
    while (!InFile.eof()) {
        InFile.getline(Str, sizeof(Str));
            if (CountWords(Str) == NumResTypes) {
            break;
        }
    }
}

static const unsigned int NUM_RES_TYPES = 21;

Rcx_Ptl * Threader::CreateRcxPtl(double weightContacts)
{
    Rcx_Ptl*  pmf;
    const char      *FileName = "ContactPotential";
    char      ResName[32];
    char      Path[512];
    unsigned int i, j, k;
    double    temp;

    static const unsigned int kNumDistances = 6;
    static const unsigned int kPeptideIndex = 20;

    /* open the contact potential for reading */
    auto_ptr<CNcbiIfstream> InFile(new CNcbiIfstream((GetDataDir()+FileName).c_str(), IOS_BASE::in));
    if (!(*InFile)) {
        ERRORMSG("Threader::CreateRcxPtl() - can't open " << Path << " for reading");
        return NULL;
    }

    pmf = NewRcxPtl(NUM_RES_TYPES, kNumDistances, kPeptideIndex);

    /* read in the contact potential */
    for (i=0; i<kNumDistances; ++i) {
        ReadToRowOfEnergies(*InFile, NUM_RES_TYPES);
        if (InFile->eof()) goto error;
        for (j=0; j<NUM_RES_TYPES; ++j) {
            InFile->getline(ResName, sizeof(ResName), ' ');  /* skip residue name */
            if (InFile->eof()) goto error;
            for (k=0; k<NUM_RES_TYPES; ++k) {
                *InFile >> temp;
                if (InFile->eof()) goto error;
                pmf->rre[i][j][k] = ThrdRound(temp*SCALING_FACTOR*weightContacts);
            }
        }
    }

    /* read in the hydrophobic energies */
    ReadToRowOfEnergies(*InFile, kNumDistances);
    for (i=0; i<NUM_RES_TYPES; ++i) {
        InFile->getline(ResName, sizeof(ResName), ' ');  /* skip residue name */
        if (InFile->eof()) goto error;
        for (j=0; j<kNumDistances; ++j) {
            *InFile >> temp;
            if (InFile->eof()) goto error;
            pmf->re[j][i] = ThrdRound(temp*SCALING_FACTOR*weightContacts);
        }
    }

    /* calculate sum of pair energies plus hydrophobic energies */
    for(i=0; i<kNumDistances; ++i) {
        for(j=0; j<NUM_RES_TYPES; ++j) {
            for(k=0; k<NUM_RES_TYPES; ++k) {
                pmf->rrt[i][j][k] = pmf->rre[i][j][k] + pmf->re[i][j] + pmf->re[i][k];
            }
        }
    }

    return(pmf);

error:
    ERRORMSG("Threader::CreateRcxPtl() - error parsing " << FileName);
    FreeRcxPtl(pmf);
    return NULL;
}

/*----------------------------------------------------------------------------
 *  set up the annealing parameters. (more code swiped from DDV)
 *  hard-coded for now.  later we can move these parameters to a file.
 *---------------------------------------------------------------------------*/
Gib_Scd * Threader::CreateGibScd(bool fast, unsigned int nRandomStarts)
{
    Gib_Scd*  gsp;
    unsigned int       NumTrajectoryPoints;

    static const unsigned int kNumTempSteps = 3;

    gsp = NewGibScd(kNumTempSteps);

    gsp->nrs = nRandomStarts;     /* Number of random starts */
    gsp->nts = kNumTempSteps;     /* Number of temperature steps */
    gsp->crs = 50;                /* Number of starts before convergence test */
    gsp->cfm = 20;                /* Top thread frequency convergence criterion */
    gsp->csm = 5;                 /* Top thread start convergence criterion */
    gsp->cet = 5 * SCALING_FACTOR;/* Temperature for convergence test ensemble */
    gsp->cef = 50;                /* Percent of ensemble defining top threads */
    gsp->isl = 1;                 /* Code for choice of starting locations */
    gsp->iso = 0;                 /* Code for choice of segment sample order */
    gsp->ito = 0;                 /* Code for choice of terminus sample order */
    gsp->rsd = -1;                /* Seed for random number generator -- neg for Rand01() */
    gsp->als = 0;                 /* Code for choice of alignment record style */
    gsp->trg = 0;                 /* Code for choice of trajectory record */

    if (fast) {
    //    gsp->nti[0] = 5;            /* Number of iterations per tempeature step */
    //    gsp->nti[1] = 10;
    //    gsp->nti[2] = 25;
        gsp->nti[0] = 10;           /* Number of iterations per tempeature step */
        gsp->nti[1] = 20;
        gsp->nti[2] = 40;
    } else {
        gsp->nti[0] = 10;           /* Number of iterations per tempeature step */
        gsp->nti[1] = 20;
        gsp->nti[2] = 40;
    }

    gsp->nac[0] = 4;              /* Number of alignment cycles per iteration */
    gsp->nac[1] = 4;
    gsp->nac[2] = 4;

    gsp->nlc[0] = 2;              /* Number of location cycles per iteration */
    gsp->nlc[1] = 2;
    gsp->nlc[2] = 2;

    if (fast) {
    //    gsp->tma[0] = 5 * SCALING_FACTOR;  /* Temperature steps for alignment sampling */
    //    gsp->tma[1] = 5 * SCALING_FACTOR;
    //    gsp->tma[2] = 5 * SCALING_FACTOR;
        gsp->tma[0] = 20 * SCALING_FACTOR;  /* Temperature steps for alignment sampling */
        gsp->tma[1] = 10 * SCALING_FACTOR;
        gsp->tma[2] =  5 * SCALING_FACTOR;
    } else {
        gsp->tma[0] = 20 * SCALING_FACTOR;  /* Temperature steps for alignment sampling */
        gsp->tma[1] = 10 * SCALING_FACTOR;
        gsp->tma[2] =  5 * SCALING_FACTOR;
    }

    gsp->tml[0] = 5 * SCALING_FACTOR;     /* Temperature steps for location sampling */
    gsp->tml[1] = 5 * SCALING_FACTOR;
    gsp->tml[2] = 5 * SCALING_FACTOR;

    gsp->lms[0] = 0;              /* Iterations before local minimum test */
    gsp->lms[1] = 10;
    gsp->lms[2] = 20;

    gsp->lmw[0] = 0;              /* Iterations in local min test interval */
    gsp->lmw[1] = 10;
    gsp->lmw[2] = 10;

    gsp->lmf[0] = 0;              /* Percent of top score indicating local min */
    gsp->lmf[1] = 80;
    gsp->lmf[2] = 95;

    if (gsp->als == 0) {
        NumTrajectoryPoints = 1;
    } else {
        NumTrajectoryPoints = 0;
        for (unsigned int i=0; i<kNumTempSteps; ++i) {
            NumTrajectoryPoints += gsp->nti[i] * (gsp->nac[i] + gsp->nlc[i]);
        }
        NumTrajectoryPoints *= gsp->nrs;
        NumTrajectoryPoints += gsp->nrs;
    }
    gsp->ntp = NumTrajectoryPoints;

    return(gsp);
}

#define NO_VIRTUAL_COORDINATE(coord) \
    do { coord->type = Threader::MISSING_COORDINATE; return; } while (0)

static void GetVirtualResidue(const AtomSet *atomSet, const Molecule *mol,
    const Residue *res, Threader::VirtualCoordinate *coord)
{
    // find coordinates of key atoms
    const AtomCoord *C = NULL, *CA = NULL, *CB = NULL, *N = NULL;
    Residue::AtomInfoMap::const_iterator a, ae = res->GetAtomInfos().end();
    for (a=res->GetAtomInfos().begin(); a!=ae; ++a) {
        AtomPntr ap(mol->id, res->id, a->first);
        if (a->second->atomicNumber == 6) {
            if (a->second->code == " C  ")
                C = atomSet->GetAtom(ap, true, true);
            else if (a->second->code == " CA ")
                CA = atomSet->GetAtom(ap, true, true);
            else if (a->second->code == " CB ")
                CB = atomSet->GetAtom(ap, true, true);
        } else if (a->second->atomicNumber == 7 && a->second->code == " N  ")
            N = atomSet->GetAtom(ap, true, true);
        if (C && CA && CB && N) break;
    }
    if (!C || !CA || !N) NO_VIRTUAL_COORDINATE(coord);

    // find direction of real or idealized C-beta
    Vector toCB;

    // if C-beta present, vector is in its direction
    if (CB) {
        toCB = CB->site - CA->site;
    }

    // ... else need to calculate a C-beta direction (not C-beta position!)
    else {
        Vector CaN, CaC, cross, bisect;
        CaN = N->site - CA->site;
        CaC = C->site - CA->site;
        // for a true bisector, these vectors should be normalized! but they aren't in other
        // versions of the threader (Cn3D/C and S), so the average is used instead...
//        CaN.normalize();
//        CaC.normalize();
        bisect = CaN + CaC;
        bisect.normalize();
        cross = vector_cross(CaN, CaC);
        cross.normalize();
        toCB = 0.816497 * cross - 0.57735 * bisect;
    }

    // virtual C-beta location is 2.4 A away from C-alpha in the C-beta direction
    toCB.normalize();
    coord->coord = CA->site + 2.4 * toCB;
    coord->type = Threader::VIRTUAL_RESIDUE;

    // is this disulfide-bound?
    Molecule::DisulfideMap::const_iterator ds = mol->disulfideMap.find(res->id);
    coord->disulfideWith =
        (ds == mol->disulfideMap.end()) ? -1 :
        (ds->second - 1) * 2;   // calculate virtualCoordinate index from other residueID
}

static void GetVirtualPeptide(const AtomSet *atomSet, const Molecule *mol,
    const Residue *res1, const Residue *res2, Threader::VirtualCoordinate *coord)
{
    if (res1->alphaID == Residue::NO_ALPHA_ID || res2->alphaID == Residue::NO_ALPHA_ID)
        NO_VIRTUAL_COORDINATE(coord);

    AtomPntr ap1(mol->id, res1->id, res1->alphaID), ap2(mol->id, res2->id, res2->alphaID);
    const AtomCoord
        *atom1 = atomSet->GetAtom(ap1, true, true),   // 'true' means just use first alt coord
        *atom2 = atomSet->GetAtom(ap2, true, true);
    if (!atom1 || !atom2) NO_VIRTUAL_COORDINATE(coord);

    coord->coord = (atom1->site + atom2->site) / 2;
    coord->type = Threader::VIRTUAL_PEPTIDE;
    coord->disulfideWith = -1;
}

static void GetVirtualCoordinates(const Molecule *mol, const AtomSet *atomSet,
    Threader::VirtualCoordinateList *virtualCoordinates)
{
    virtualCoordinates->resize(2 * mol->residues.size() - 1);
    Molecule::ResidueMap::const_iterator r, re = mol->residues.end();
    const Residue *prevResidue = NULL;
    unsigned int i = 0;
    for (r=mol->residues.begin(); r!=re; ++r) {
        if (prevResidue)
            GetVirtualPeptide(atomSet, mol,
                prevResidue, r->second, &((*virtualCoordinates)[i++]));
        prevResidue = r->second;
        GetVirtualResidue(atomSet, mol,
            r->second, &((*virtualCoordinates)[i++]));
    }
}

static const unsigned int MAX_DISTANCE_BIN = 5;
static unsigned int BinDistance(const Vector& p1, const Vector& p2)
{
    double dist = (p2 - p1).length();
    unsigned int bin;

    if (dist > 10.0)
        bin = MAX_DISTANCE_BIN + 1;
    else if (dist > 9.0)
        bin = 5;
    else if (dist > 8.0)
        bin = 4;
    else if (dist > 7.0)
        bin = 3;
    else if (dist > 6.0)
        bin = 2;
    else if (dist > 5.0)
        bin = 1;
    else
        bin = 0;

    return bin;
}

static void GetContacts(const Threader::VirtualCoordinateList& coords,
    Threader::ContactList *resResContacts, Threader::ContactList *resPepContacts)
{
    unsigned int i, j, bin;

    // loop i through whole chain, just to report all missing coords
    for (i=0; i<coords.size(); ++i) {
        if (coords[i].type == Threader::MISSING_COORDINATE) {
            WARNINGMSG("Threader::CreateFldMtf() - unable to determine virtual coordinate for "
                << ((i%2 == 0) ? "sidechain " : "peptide ") << (i/2));
            continue;
        }

        for (j=i+10; j<coords.size(); ++j) {    // must be at least 10 virtual bonds away

            if (coords[j].type == Threader::MISSING_COORDINATE ||
                // not interested in peptide-peptide contacts
                (coords[i].type == Threader::VIRTUAL_PEPTIDE &&
                 coords[j].type == Threader::VIRTUAL_PEPTIDE) ||
                // don't include disulfide-bonded cysteine pairs
                (coords[i].disulfideWith == (int)j || coords[j].disulfideWith == (int)i)
                ) continue;

            bin = BinDistance(coords[i].coord, coords[j].coord);
            if (bin <= MAX_DISTANCE_BIN) {
                // add residue-residue contact - res1 is lower-numbered residue
                if (coords[i].type == Threader::VIRTUAL_RESIDUE &&
                    coords[j].type == Threader::VIRTUAL_RESIDUE) {
                    resResContacts->resize(resResContacts->size() + 1);
                    resResContacts->back().vc1 = i;
                    resResContacts->back().vc2 = j;
                    resResContacts->back().distanceBin = bin;
                }
                // add residue-peptide contact
                else {
                    resPepContacts->resize(resPepContacts->size() + 1);
                    resPepContacts->back().distanceBin = bin;
                    // peptide must go in vc2
                    if (coords[i].type == Threader::VIRTUAL_RESIDUE) {
                        resPepContacts->back().vc1 = i;
                        resPepContacts->back().vc2 = j;
                    } else {
                        resPepContacts->back().vc2 = i;
                        resPepContacts->back().vc1 = j;
                    }
                }
            }
        }
    }
}

static void TranslateContacts(const Threader::ContactList& resResContacts,
    const Threader::ContactList& resPepContacts, Fld_Mtf *fldMtf)
{
    unsigned int i;
    Threader::ContactList::const_iterator c;
    for (i=0, c=resResContacts.begin(); i<resResContacts.size(); ++i, ++c) {
        fldMtf->rrc.r1[i] = c->vc1 / 2;  // threader coord points to (res,pep) pair
        fldMtf->rrc.r2[i] = c->vc2 / 2;
        fldMtf->rrc.d[i] = c->distanceBin;
    }
    for (i=0, c=resPepContacts.begin(); i<resPepContacts.size(); ++i, ++c) {
        fldMtf->rpc.r1[i] = c->vc1 / 2;
        fldMtf->rpc.p2[i] = c->vc2 / 2;
        fldMtf->rpc.d[i] = c->distanceBin;
    }
}

// for sorting contacts
inline bool operator < (const Threader::Contact& c1, const Threader::Contact& c2)
{
    return (c1.vc1 < c2.vc1 || (c1.vc1 == c2.vc1 && c1.vc2 < c2.vc2));
}

static void GetMinimumLoopLengths(const Molecule *mol, const AtomSet *atomSet, Fld_Mtf *fldMtf)
{
    unsigned int i, j;
    const AtomCoord *a1, *a2;
    Molecule::ResidueMap::const_iterator r1, r2, re = mol->residues.end();
    for (r1=mol->residues.begin(), i=0; r1!=re; ++r1, ++i) {

        if (r1->second->alphaID == Residue::NO_ALPHA_ID)
            a1 = NULL;
        else {
            AtomPntr ap1(mol->id, r1->second->id, r1->second->alphaID);
            a1 = atomSet->GetAtom(ap1, true, true);   // 'true' means just use first alt coord
        }

        for (r2=r1, j=i; r2!=re; ++r2, ++j) {

            if (i == j) {
                fldMtf->mll[i][j] = 0;
            } else {
                if (r2->second->alphaID == Residue::NO_ALPHA_ID)
                    a2 = NULL;
                else {
                    AtomPntr ap2(mol->id, r2->second->id, r2->second->alphaID);
                    a2 = atomSet->GetAtom(ap2, true, true);
                }
                fldMtf->mll[i][j] = fldMtf->mll[j][i] =
                    (!a1 || !a2) ? 0 : (int) (((a2->site - a1->site).length() - 2.7) / 3.4);
            }
        }
    }
}

Fld_Mtf * Threader::CreateFldMtf(const Sequence *masterSequence)
{
    if (!masterSequence) return NULL;

    const Molecule *mol = masterSequence->molecule;

    // return cached copy if we've already constructed a Fld_Mtf for this master
    ContactMap::iterator c = mol ? contacts.find(mol) : contacts.find(masterSequence);
    if (c != contacts.end()) return c->second;

    // work-around to allow PSSM-only threading when master has no structure (or only C-alphas)
    Fld_Mtf *fldMtf;
    if (!mol || mol->parentSet->isAlphaOnly) {
        fldMtf = NewFldMtf(masterSequence->Length(), 0, 0);
        contacts[masterSequence] = fldMtf;
        return fldMtf;
    }

    // for convenience so subroutines don't have to keep looking this up... Use first
    // CoordSet if multiple model (e.g., NMR)
    const StructureObject *object;
    if (!mol->GetParentOfType(&object)) return NULL;
    const AtomSet *atomSet = object->coordSets.front()->atomSet;

    // get virtual coordinates for this chain
    VirtualCoordinateList virtualCoordinates;
    GetVirtualCoordinates(mol, atomSet, &virtualCoordinates);

    // check for contacts of virtual coords separated by >= 10 virtual bonds
    ContactList resResContacts, resPepContacts;
    GetContacts(virtualCoordinates, &resResContacts, &resPepContacts);

    // create Fld_Mtf, and store contacts in it
    fldMtf = NewFldMtf(mol->residues.size(), resResContacts.size(), resPepContacts.size());
    resPepContacts.sort();  // not really necessary, but makes same order as Cn3D for comparison/testing
    TranslateContacts(resResContacts, resPepContacts, fldMtf);

    // fill out min. loop lengths
    GetMinimumLoopLengths(mol, atomSet, fldMtf);

    TRACEMSG("created Fld_Mtf for " << mol->identifier->pdbID << " chain '" << (char) mol->identifier->pdbChain << "'");
    contacts[mol] = fldMtf;
    return fldMtf;
}

static BlockMultipleAlignment * CreateAlignmentFromThdTbl(const Thd_Tbl *thdTbl, unsigned int nResult,
    const Cor_Def *corDef, BlockMultipleAlignment::SequenceList *sequences, AlignmentManager *alignmentManager)
{
    if (corDef->sll.n != thdTbl->nsc || (int)nResult >= thdTbl->n) {
        ERRORMSG("CreateAlignmentFromThdTbl() - inconsistent Thd_Tbl");
        return NULL;
    }

    BlockMultipleAlignment *newAlignment = new BlockMultipleAlignment(sequences, alignmentManager);

    // add blocks from threader result
    for (int block=0; block<corDef->sll.n; ++block) {
        UngappedAlignedBlock *aBlock = new UngappedAlignedBlock(newAlignment);
        aBlock->SetRangeOfRow(0,
            corDef->sll.rfpt[block] - thdTbl->no[block][nResult],
            corDef->sll.rfpt[block] + thdTbl->co[block][nResult]);
        aBlock->SetRangeOfRow(1,
            thdTbl->al[block][nResult] - thdTbl->no[block][nResult],
            thdTbl->al[block][nResult] + thdTbl->co[block][nResult]);
        aBlock->width = thdTbl->no[block][nResult] + 1 + thdTbl->co[block][nResult];
        if (!newAlignment->AddAlignedBlockAtEnd(aBlock)) {
            ERRORMSG("CreateAlignmentFromThdTbl() - error adding block");
            delete newAlignment;
            return NULL;
        }
    }

    // finish alignment
    if (!newAlignment->AddUnalignedBlocks() || !newAlignment->UpdateBlockMapAndColors()) {
        ERRORMSG("CreateAlignmentFromThdTbl() - error finishing alignment");
        delete newAlignment;
        return NULL;
    }

    return newAlignment;
}

static bool FreezeIsolatedBlocks(Cor_Def *corDef, const Cor_Def *masterCorDef, const Qry_Seq *qrySeq)
{
    if (!corDef || !masterCorDef || !qrySeq ||
        corDef->sll.n != masterCorDef->sll.n || corDef->sll.n != qrySeq->sac.n) {
        ERRORMSG("FreezeIsolatedBlocks() - bad parameters");
        return false;
    }

    TRACEMSG("freezing blocks...");
    for (int i=0; i<corDef->sll.n; ++i) {

        // default: blocks allowed to grow
        corDef->sll.nomx[i] = masterCorDef->sll.nomx[i];
        corDef->sll.comx[i] = masterCorDef->sll.comx[i];

        // new blocks always allowed to grow
        if (qrySeq->sac.mn[i] < 0 || qrySeq->sac.mx[i] < 0) continue;

        // if an existing block is adjacent to any new (to-be-realigned) block, then allow block's
        // boundaries to grow on that side; otherwise, freeze (isolated) existing block boundaries
        bool adjacentLeft = (i > 0 && (qrySeq->sac.mn[i - 1] < 0 || qrySeq->sac.mx[i - 1] < 0));
        bool adjacentRight = (i < corDef->sll.n - 1 &&
            (qrySeq->sac.mn[i + 1] < 0 || qrySeq->sac.mx[i + 1] < 0));

        if (!adjacentLeft) {
            corDef->sll.nomx[i] = corDef->sll.nomn[i];
//            TESTMSG("block " << i << " fixed N-terminus");
        }
        if (!adjacentRight) {
            corDef->sll.comx[i] = corDef->sll.comn[i];
//            TESTMSG("block " << i << " fixed C-terminus");
        }
    }

    return true;
}

bool Threader::Realign(const ThreaderOptions& options, BlockMultipleAlignment *masterMultiple,
    const AlignmentList *originalAlignments, AlignmentList *newAlignments,
    unsigned int *nRowsAddedToMultiple, SequenceViewer *sequenceViewer)
{
    *nRowsAddedToMultiple = 0;
    if (!masterMultiple || !originalAlignments || !newAlignments || originalAlignments->size() == 0)
        return false;

    // either calculate no z-scores (0), or calculate z-score for best result (1)
    static const unsigned int zscs = 0;

    Seq_Mtf *seqMtf = NULL;
    Cor_Def *corDef = NULL, *masterCorDef = NULL;
    Rcx_Ptl *rcxPtl = NULL;
    Gib_Scd *gibScd = NULL;
    Fld_Mtf *fldMtf = NULL;
    float *trajectory = NULL;
    bool retval = false;

    AlignmentList::const_iterator p, pe = originalAlignments->end();

#ifdef DEBUG_THREADER
    FILE *pFile;
#endif

    // create contact lists
    if (options.weightPSSM < 1.0 && (!masterMultiple->GetMaster()->molecule ||
            masterMultiple->GetMaster()->molecule->parentSet->isAlphaOnly)) {
        ERRORMSG("Can't use contact potential on non-structured master, or alpha-only (virtual bond) models!");
        goto cleanup;
    }
    if (!(fldMtf = CreateFldMtf(masterMultiple->GetMaster()))) goto cleanup;

    // create potential and Gibbs schedule
    if (!(rcxPtl = CreateRcxPtl(1.0 - options.weightPSSM))) goto cleanup;
    if (!(gibScd = CreateGibScd(true, options.nRandomStarts))) goto cleanup;
    trajectory = new float[gibScd->ntp];

    // create initial PSSM
    if (!(seqMtf = CreateSeqMtf(masterMultiple, options.weightPSSM))) goto cleanup;
#ifdef DEBUG_THREADER
    pFile = fopen("Seq_Mtf.debug.txt", "w");
    PrintSeqMtf(seqMtf, pFile);
    fclose(pFile);
#endif

    // create core definition
    if (!(corDef = CreateCorDef(masterMultiple, options.loopLengthMultiplier))) goto cleanup;
    if (options.freezeIsolatedBlocks)   // make a copy to used as an original "master"
        if (!(masterCorDef = CreateCorDef(masterMultiple, options.loopLengthMultiplier))) goto cleanup;

#ifdef DEBUG_THREADER
    pFile = fopen("Fld_Mtf.debug.txt", "w");
    PrintFldMtf(fldMtf, pFile);
    fclose(pFile);
#endif

    for (p=originalAlignments->begin(); p!=pe; ) {

        if ((*p)->NRows() != 2 || (*p)->GetMaster() != masterMultiple->GetMaster()) {
            ERRORMSG("Threader::Realign() - bad pairwise alignment");
            continue;
        }

        Qry_Seq *qrySeq = NULL;
        Thd_Tbl *thdTbl = NULL;
        unsigned int success = 0;

        // create query sequence
        if (!(qrySeq = CreateQrySeq(masterMultiple, *p, options.terminalResidueCutoff))) goto cleanup2;
#ifdef DEBUG_THREADER
        pFile = fopen("Qry_Seq.debug.txt", "w");
        PrintQrySeq(qrySeq, pFile);
        fclose(pFile);
#endif

        // freeze block sizes if opted (changes corDef but not masterCorDef or qrySeq)
        if (options.freezeIsolatedBlocks)
            FreezeIsolatedBlocks(corDef, masterCorDef, qrySeq);
#ifdef DEBUG_THREADER
        pFile = fopen("Cor_Def.debug.txt", "w");
        PrintCorDef(corDef, pFile);
        fclose(pFile);
#endif

        // create results storage structure
        thdTbl = NewThdTbl(options.nResultAlignments, corDef->sll.n);

        // actually run the threader (finally!)
        INFOMSG("threading " << (*p)->GetSequenceOfRow(1)->identifier->ToString());
        success = atd(fldMtf, corDef, qrySeq, rcxPtl, gibScd, thdTbl, seqMtf,
            trajectory, zscs, SCALING_FACTOR, (float) options.weightPSSM);

        BlockMultipleAlignment *newAlignment;
        if (success) {
            TRACEMSG("threading succeeded");
#ifdef DEBUG_THREADER
            pFile = fopen("Thd_Tbl.debug.txt", "w");
            PrintThdTbl(thdTbl, pFile);
            fclose(pFile);
#endif
            // create new alignment(s) from threading result; merge or add to list as appropriate
            for (int i=0; i<thdTbl->n; ++i) {

                // skip if this entry is not a real result
                if (thdTbl->tf[i] <= 0) continue;

                BlockMultipleAlignment::SequenceList *sequences = new BlockMultipleAlignment::SequenceList(2);
                sequences->front() = (*p)->GetMaster();
                sequences->back() = (*p)->GetSequenceOfRow(1);
                newAlignment = CreateAlignmentFromThdTbl(thdTbl, i, corDef, sequences, alignmentManager);
                if (!newAlignment) continue;

                // set scores to show in alignment
                newAlignment->SetRowDouble(0, thdTbl->tg[i]);
                newAlignment->SetRowDouble(1, thdTbl->tg[i]);
                string status = string("Threading successful; alignment score before merge: ") + NStr::DoubleToString(thdTbl->tg[i]);
                newAlignment->SetRowStatusLine(0, status);
                newAlignment->SetRowStatusLine(1, status);

                if (options.mergeAfterEachSequence) {
                    if (!sequenceViewer->EditorIsOn())
                        sequenceViewer->TurnOnEditor();
                    if (masterMultiple->MergeAlignment(newAlignment)) {
                        delete newAlignment; // if merge is successful, we can delete this alignment;
                        newAlignment = NULL;
                        ++(*nRowsAddedToMultiple);
                    }
                }

                // no merge or merge failed - add new alignment to list, let calling function deal with it
                if (newAlignment)
                    newAlignments->push_back(newAlignment);
            }
        }

        // threading failed - add old alignment to list so it doesn't get lost
        else {
            TRACEMSG("threading failed!");
            newAlignment = (*p)->Clone();
            newAlignment->SetRowDouble(0, -1.0);
            newAlignment->SetRowDouble(1, -1.0);
            newAlignment->SetRowStatusLine(0, "Threading failed!");
            newAlignment->SetRowStatusLine(1, "Threading failed!");
            newAlignments->push_back(newAlignment);
        }

cleanup2:
        if (qrySeq) FreeQrySeq(qrySeq);
        if (thdTbl) FreeThdTbl(thdTbl);

        ++p;
        if (success && p != pe && options.mergeAfterEachSequence) {
            // re-create PSSM after each merge
            FreeSeqMtf(seqMtf);
            if (!(seqMtf = CreateSeqMtf(masterMultiple, options.weightPSSM))) goto cleanup;
        }
    }

    retval = true;

cleanup:
    if (seqMtf) FreeSeqMtf(seqMtf);
    if (corDef) FreeCorDef(corDef);
    if (masterCorDef) FreeCorDef(masterCorDef);
    if (rcxPtl) FreeRcxPtl(rcxPtl);
    if (gibScd) FreeGibScd(gibScd);
    if (trajectory) delete[] trajectory;

    return retval;
}

static double CalculatePSSMScore(const BlockMultipleAlignment::UngappedAlignedBlockList& aBlocks,
    unsigned int row, const vector < int >& residueNumbers, const Seq_Mtf *seqMtf)
{
    double score = 0.0;
    BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator b, be = aBlocks.end();
    const Block::Range *masterRange, *dependentRange;
    unsigned int i;

    for (b=aBlocks.begin(); b!=be; ++b) {
        masterRange = (*b)->GetRangeOfRow(0);
        dependentRange = (*b)->GetRangeOfRow(row);
        for (i=0; i<(*b)->width; ++i)
            if (residueNumbers[dependentRange->from + i] >= 0)
                score += seqMtf->ww[masterRange->from + i][residueNumbers[dependentRange->from + i]];
    }

//    TESTMSG("PSSM score for row " << row << ": " << score);
    return score;
}

static double CalculateContactScore(const BlockMultipleAlignment *multiple,
    unsigned int row, const vector < int >& residueNumbers, const Fld_Mtf *fldMtf, const Rcx_Ptl *rcxPtl)
{
    double score = 0.0;
    int seqIndex1, seqIndex2, resNum1, resNum2, dist, i;

    // for each res-res contact, convert seqIndexes of master into corresponding seqIndexes
    // of dependent if they're aligned; add contact energies if so
    for (i=0; i<fldMtf->rrc.n; ++i) {
        seqIndex1 = multiple->GetAlignedDependentIndex(fldMtf->rrc.r1[i], row);
        if (seqIndex1 < 0) continue;
        seqIndex2 = multiple->GetAlignedDependentIndex(fldMtf->rrc.r2[i], row);
        if (seqIndex2 < 0) continue;

        resNum1 = residueNumbers[seqIndex1];
        resNum2 = residueNumbers[seqIndex2];
        if (resNum1 < 0 || resNum2 < 0) continue;

        dist = fldMtf->rrc.d[i];
        score += rcxPtl->rre[dist][resNum1][resNum2] + rcxPtl->re[dist][resNum1] + rcxPtl->re[dist][resNum2];
    }

    // ditto for res-pep contacts - except only one dependent residue to look up; 2nd is always peptide group
    for (i=0; i<fldMtf->rpc.n; ++i) {
        seqIndex1 = multiple->GetAlignedDependentIndex(fldMtf->rpc.r1[i], row);
        if (seqIndex1 < 0) continue;

        // peptides are only counted if both contributing master residues are aligned
        if (fldMtf->rpc.p2[i] >= (int)multiple->GetMaster()->Length() - 1 ||
            !multiple->IsAligned(0U, fldMtf->rpc.p2[i]) ||
            !multiple->IsAligned(0U, fldMtf->rpc.p2[i] + 1)) continue;

        resNum1 = residueNumbers[seqIndex1];
        if (resNum1 < 0) continue;
        resNum2 = NUM_RES_TYPES - 1; // peptide group

        dist = fldMtf->rpc.d[i];
        score += rcxPtl->rre[dist][resNum1][resNum2] + rcxPtl->re[dist][resNum1] + rcxPtl->re[dist][resNum2];
    }

//    TESTMSG("Contact score for row " << row << ": " << score);
    return score;
}

bool Threader::CalculateScores(const BlockMultipleAlignment *multiple, double weightPSSM)
{
    Seq_Mtf *seqMtf = NULL;
    Rcx_Ptl *rcxPtl = NULL;
    Fld_Mtf *fldMtf = NULL;
    BlockMultipleAlignment::UngappedAlignedBlockList aBlocks;
    vector < int > residueNumbers;
    bool retval = false;
    unsigned int row;

    // create contact lists
    if (weightPSSM < 1.0 && (!multiple->GetMaster()->molecule ||
            multiple->GetMaster()->molecule->parentSet->isAlphaOnly)) {
        ERRORMSG("Can't use contact potential on non-structured master, or alpha-only (virtual bond) models!");
        goto cleanup;
    }
    if (weightPSSM < 1.0 && !(fldMtf = CreateFldMtf(multiple->GetMaster()))) goto cleanup;

    // create PSSM
    if (weightPSSM > 0.0 && !(seqMtf = CreateSeqMtf(multiple, weightPSSM))) goto cleanup;

    // create potential
    if (weightPSSM < 1.0 && !(rcxPtl = CreateRcxPtl(1.0 - weightPSSM))) goto cleanup;

    // get aligned blocks
    multiple->GetUngappedAlignedBlocks(&aBlocks);

    for (row=0; row<multiple->NRows(); ++row) {

        // get sequence's residue numbers
        const Sequence *seq = multiple->GetSequenceOfRow(row);
        residueNumbers.resize(seq->Length());
        for (unsigned int i=0; i<seq->Length(); ++i)
            residueNumbers[i] = LookupThreaderResidueNumberFromCharacterAbbrev(seq->sequenceString[i]);

        // sum score types (weightPSSM already built into seqMtf & rcxPtl)
        double
            scorePSSM = (weightPSSM > 0.0) ?
                CalculatePSSMScore(aBlocks, row, residueNumbers, seqMtf) : 0.0,
            scoreContacts = (weightPSSM < 1.0) ?
                CalculateContactScore(multiple, row, residueNumbers, fldMtf, rcxPtl) : 0.0,
            score = (scorePSSM + scoreContacts) / SCALING_FACTOR;

        // set score in alignment rows (for sorting and status line display)
        multiple->SetRowDouble(row, score);
        CNcbiOstrstream oss;
        oss << "PSSM+Contact score (PSSM x" << weightPSSM << "): " << score;
        multiple->SetRowStatusLine(row, (string) CNcbiOstrstreamToString(oss));
    }

    retval = true;

cleanup:
    if (seqMtf) FreeSeqMtf(seqMtf);
    if (rcxPtl) FreeRcxPtl(rcxPtl);
    return retval;
}

unsigned int Threader::GetGeometryViolations(const BlockMultipleAlignment *multiple,
    GeometryViolationsForRow *violations)
{
    // create contact lists
    if (!multiple->GetMaster()->molecule || multiple->GetMaster()->molecule->parentSet->isAlphaOnly) {
        ERRORMSG("Can't use contact potential on non-structured master, or alpha-only (virtual bond) models!");
        return false;
    }

    violations->clear();
    violations->resize(multiple->NRows());

    // look for too-short regions between aligned blocks in unstructured sequences, using minimum loop
    // lengths of all structured sequences
    BlockMultipleAlignment::UngappedAlignedBlockList aBlocks;
    multiple->GetUngappedAlignedBlocks(&aBlocks);
    BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator b, be = aBlocks.end(), n;
    unsigned int nViolations = 0, row;
    const Block::Range *thisRange, *nextRange;
    int minimumLoop;
    for (b=aBlocks.begin(); b!=be; ++b) {
        n = b;
        ++n;
        if (n == be) break;

        // find minimum allowed loop length between these blocks based on any structure
        minimumLoop = kMax_Int;
        for (row=0; row<multiple->NRows(); ++row) {
            if (multiple->GetSequenceOfRow(row)->molecule && !multiple->GetSequenceOfRow(row)->molecule->parentSet->isAlphaOnly) {
                thisRange = (*b)->GetRangeOfRow(row);
                nextRange = (*n)->GetRangeOfRow(row);
                Fld_Mtf *fldMtf = CreateFldMtf(multiple->GetSequenceOfRow(row));
                if (!fldMtf) {
                    ERRORMSG("Can't create FldMtf for " << multiple->GetSequenceOfRow(row)->identifier->ToString());
                    return false;
                }
                if (fldMtf->mll[thisRange->to][nextRange->from] < minimumLoop)
                    minimumLoop = fldMtf->mll[thisRange->to][nextRange->from];
            }
        }

        // check for too-short loops in all unstructured rows
        for (row=1; row<multiple->NRows(); ++row) {
            if (!multiple->GetSequenceOfRow(row)->molecule || multiple->GetSequenceOfRow(row)->molecule->parentSet->isAlphaOnly) {
                thisRange = (*b)->GetRangeOfRow(row);
                nextRange = (*n)->GetRangeOfRow(row);
                // violation found
                if (nextRange->from - thisRange->to - 1 < minimumLoop) {
                    (*violations)[row].push_back(make_pair(thisRange->to, nextRange->from));
                    ++nViolations;
                }
            }
        }
    }

//    TESTMSG("Found " << nViolations << " geometry violations");
    return nViolations;
}

unsigned int Threader::EstimateNRandomStarts(const BlockMultipleAlignment *coreAlignment,
    const BlockMultipleAlignment *toBeThreaded)
{
    unsigned int nBlocksToAlign = 0;
    BlockMultipleAlignment::UngappedAlignedBlockList multipleABlocks, pairwiseABlocks;
    coreAlignment->GetUngappedAlignedBlocks(&multipleABlocks);
    toBeThreaded->GetUngappedAlignedBlocks(&pairwiseABlocks);

    // if a block in the multiple is *not* contained in the pairwise (looking at master coords),
    // then it'll probably be realigned upon threading
    BlockMultipleAlignment::UngappedAlignedBlockList::const_iterator
        m, me = multipleABlocks.end(), p, pe = pairwiseABlocks.end();
    const Block::Range *multipleRange, *pairwiseRange;
    for (m=multipleABlocks.begin(); m!=me; ++m) {
        multipleRange = (*m)->GetRangeOfRow(0);
        bool realignBlock = true;
        for (p=pairwiseABlocks.begin(); p!=pe; ++p) {
            pairwiseRange = (*p)->GetRangeOfRow(0);
            if (pairwiseRange->from <= multipleRange->from && pairwiseRange->to >= multipleRange->to) {
                realignBlock = false;
                break;
            }
        }
        if (realignBlock) ++nBlocksToAlign;
    }

    if (nBlocksToAlign <= 1)
        return 1;
    else
        // round to nearest integer
        return (int) (exp(1.5 + 0.25432 * nBlocksToAlign) + 0.5);
}

END_SCOPE(Cn3D)
