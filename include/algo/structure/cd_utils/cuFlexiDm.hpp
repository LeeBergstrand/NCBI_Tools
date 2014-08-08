#ifndef CU_FLEXIDM_HPP
#define CU_FLEXIDM_HPP

/*  $Id: cuFlexiDm.hpp 161564 2009-05-28 19:23:37Z lanczyck $
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
* Author:  Charlie Liu
*
* File Description: 
*
*      Concrete distance matrix class.
*      Distance is computed based on pure percent pairwise AA identity in 
*      aligned blocks, with or without a correction for multiple AA
*      substitutions as per Kimura.
*
*/

#include <algo/structure/cd_utils/cuDistmat.hpp>
#include <algo/structure/cd_utils/cuResidueProfile.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
BEGIN_SCOPE(cd_utils)

struct ResidueCell
{
	ResidueCell(char aa, bool isaligned) : residue(aa), aligned(isaligned){}
	char residue;
	bool aligned;
};

class NCBI_CDUTILS_EXPORT ResidueMatrix : public ColumnReader
{
public:
	ResidueMatrix(unsigned numRows);
	void read(ColumnResidueProfile& crp);
	bool getAlignedPair(unsigned row1, unsigned row2, pair< string, string >& seqPair); 
	typedef vector< ResidueCell > RowContent;
	RowContent& getRow(int row) {return m_rows[row];}
private:
	
	vector< RowContent > m_rows;
	int m_numRows;
};

//  This class simply uses the number of AA identities in the specified region
//  to define the distance between two sequences:
//
//  d[i][j] = 1 - (n_matched/n_tested);  d is in [0, 1]

class FlexiDm : public DistanceMatrix {

    static const double MAX_DISTANCE;   
    static const EDistMethod DIST_METHOD;

public:

    FlexiDm(EScoreMatrixType type = GLOBAL_DEFAULT_SCORE_MATRIX, int uniformLength = -1);

    ~FlexiDm();
    bool ComputeMatrix(pProgressFunction pFunc);

    //  Allow the user to define a single length to use in GetPercentIdentities.
    //  This length will *not* be used if (# of identities/uniformLength) > 1.
    void SetUniformLength(int uniformLength) {m_uniformLength = (uniformLength != 0) ? uniformLength : -1;}
    int GetUniformLength() const {return m_uniformLength;}

    //     Distance is 1 - (fraction of identical residues)
    static double GetDistance(int identities, int alignment_length);

private:

    //  When positive, always use this as the length when computing distances 
    //  to force normalization to a common size alignment.  E.g., when there
    //  are normal and pending rows, one might wish to force to measure the 
    //  number of identities between pending rows relative to the length of the
    //  normal alignment.
    //  When zero or negative, this value is ignored.
    //  In addition, this length will *not* be used if (# of identities/uniformLength) > 1.
    int m_uniformLength;

    void GetPercentIdentities(pProgressFunction pFunc);
    void initDMIdentities(EScoreMatrixType type, int nExt=0, int cExt=0);
};

END_SCOPE(cd_utils)
END_NCBI_SCOPE

#endif /*  CU_FlexiDm__HPP  */
