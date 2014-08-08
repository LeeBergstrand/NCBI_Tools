#ifndef CU_SLC_TREEALG__HPP
#define CU_SLC_TREEALG__HPP

/*  $Id: cuSeqTreeSlc.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Chris Lanczycki
*
* File Description:
*   Implementation of a single-linkage clustering algorithm
*   Produces a tree with a FAKE root.
*   Estimates branch lengths as half node-to-node distance.
*
*/

#include <algo/structure/cd_utils/cuSeqTreeStream.hpp>
#include <algo/structure/cd_utils/cuSeqTreeAlg.hpp>
#include <algo/structure/cd_utils/cuDistmat.hpp>
#include <corelib/ncbistd.hpp>
#include <objects/seqloc/PDB_seq_id.hpp>
#include <objects/seqloc/PDB_mol_id.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(cd_utils)

class NCBI_CDUTILS_EXPORT SLC_TreeAlgorithm : public TreeAlgorithm {

    static const double REPLACE_NEG_DIST;
    static const Rootedness MY_ROOTEDNESS;
    static const DistanceMatrix::TMatType INIT_MINIMA;
	static const ETreeMethod MY_TREE_METHOD;

public:

    SLC_TreeAlgorithm();

    SLC_TreeAlgorithm(DistanceMatrix* dm); 

    virtual ~SLC_TreeAlgorithm();
    virtual long GetNumLoopsForTreeCalc();
    virtual void ComputeTree(SeqTree* tree, pProgressFunction pFunc);
    virtual string toString();
//    string toString(const TSeqIt& sit);

    //virtual void SetCDD(CCd* cdd);
    virtual void SetDistMat(DistanceMatrix* dm);


private:

    static const int USED_ROW;
    static const int BAD_INDEX;

    void initializeNodes();
    bool isHub(const TSeqIt& sit);
    bool isInternal(const TSeqIt& sit);

    int  numChildren(const TSeqIt& sit);

    void Join(int inode1, int inode2, double len1, double len2);  

    //  SLC method data members.

    int m_nseqs;                 //  Number of sequences (dimension of m_dm)
    int m_nextNode;              //  Index of next free node in m_seqiters.
    vector< SeqItem* > m_items;  //  Contents of a tree node.
    TTreeIt  m_seqiters;         //  List of iterators pointing to tree nodes,
                                 //  identified using the row id from alignment provided.

};

//  Hub node is node 0.
inline
bool SLC_TreeAlgorithm::isHub(const TSeqIt& it) {
    return (it == m_seqiters[0]);
}

inline
bool SLC_TreeAlgorithm::isInternal(const TSeqIt& it) {
    return (it.is_valid() && !isHub(it));
}

END_SCOPE(cd_utils)
END_NCBI_SCOPE

#endif   /*  CU_SLC_TREEALG__HPP  */
