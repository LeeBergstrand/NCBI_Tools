#ifndef ALGO_BLAST_GUMBEL_PARAMS__GUMBEL_PARAMS___HPP
#define ALGO_BLAST_GUMBEL_PARAMS__GUMBEL_PARAMS___HPP

/* $Id: gumbel_params.hpp 389681 2013-02-20 13:16:20Z kornbluh $
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
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
* ===========================================================================*/

/*****************************************************************************

File name: gumbel_params.hpp

Author: Greg Boratyn, Sergey Sheetlin

Contents: Wrapper classes for real time Gumbel parameters computing code

******************************************************************************/

#include <corelib/ncbiobj.hpp>
#include <algo/blast/gumbel_params/general_score_matrix.hpp>
#include <vector>


BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

class CGumbelParamsRandDiagnostics;


/// Input parameters for Gumbel parameters calculation
///
class CGumbelParamsOptions : public CObject
{
public:
    typedef vector<double> TFrequencies;

public:
    
    //---- Constructors ----//

    /// Create empty options object
    ///
    CGumbelParamsOptions(void);


    /// Create options with given score matrix and residue probabilities
    /// @param smatrix Score matrix values [in]
    /// @param seq1_residue_probs Residue probabilities for sequence 1 [in]
    /// @param seq2_residue_probs Residue probabilities for sequence 2 [in]
    ///
    /// The score matrix can be non-symmetric
    CGumbelParamsOptions(const CGeneralScoreMatrix* smatrix,
                         const vector<double>& seq1_residue_probs,
                         const vector<double>& seq2_residue_rpobs);

    /// Copy constructor
    /// @param  options Options object [in]
    ///
    CGumbelParamsOptions(const CGumbelParamsOptions& options);


    //---- Getters and Setters ----//
    
    /// Set the value of gap opening penalty
    /// @param g_open Gap opening penalty [in]
    ///
    void SetGapOpening(Int4 g_open) {m_GapOpening = g_open;}

    /// Get gap opening penalty
    /// @return Gap opening penalty
    ///
    Int4 GetGapOpening(void) const {return m_GapOpening;}

    /// Set gap extention penalty
    /// @param g_exten Gap extension penalty [in]
    ///
    void SetGapExtension(Int4 g_exten) {m_GapExtension = g_exten;}

    /// Get gap extention penalty
    /// @return Gap extension penalty
    ///
    Int4 GetGapExtension(void) const {return m_GapExtension;}

    /// Set relative error threshold for lambda parameter calculation
    /// for gapped aligmment
    /// @param lambda_err Relative error threshold [in]
    ///
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.
    void SetLambdaAccuracy(double lambda_err) 
    {m_LambdaAccuracy = lambda_err;}

    /// Get relative error threshold for lambda parameter calculation
    /// for gapped aligmment
    /// @return Relative error threshold for lambda calculation
    ///
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.
    double GetLambdaAccuracy(void) const {return m_LambdaAccuracy;}

    /// Set relative error threshold for K parameter calculation for gapped
    /// alignment
    /// @param k_err Relative error threshold [in]
    ///
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.
    void SetKAccuracy(double k_err) {m_KAccuracy = k_err;}

    /// Get relative error threshold for K parameter calculation for gapped
    /// alignment
    /// @return Relative error threshold for K parameter calculation
    ///
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.    
    double GetKAccuracy(void) const {return m_KAccuracy;}

    /// Set gapped/gapless regime
    /// @param gapped Regime (gapped if true, gapless otherwise) [in]
    ///
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.
    void SetGapped(bool gapped) {m_IsGapped = gapped;}

    /// Get gapped/gapless regime
    /// @return true if gapped, false otherwise
    ///
    bool GetGapped(void) const {return m_IsGapped;}

    /// Set score matrix
    /// @param smatrix Score matrix values [in]
    ///
    void SetScoreMatrix(const CGeneralScoreMatrix& smatrix);

    /// Set score matrix
    /// @param smatrix Score matrix [in]
    ///
    void SetScoreMatrix(const CRef<CGeneralScoreMatrix>& smatrix);

    /// Get score matrix
    /// @return Pointer to score matrix
    ///
    const Int4** GetScoreMatrix(void) const 
    {return (*m_ScoreMatrix).GetMatrix();}

    /// Set residue probabilities for sequence 1
    /// @param probs Residue probabilities [in]
    ///
    void SetSeq1ResidueProbs(const TFrequencies& probs) 
    {m_Seq1ResidueProbs = probs;}


    /// Get sequence 1 residue probabilities
    /// @return Residue probabilities for sequence 1
    ///
    const TFrequencies& GetSeq1ResidueProbs(void) const 
    {return m_Seq1ResidueProbs;}
        
    /// Set residue probabilities for sequence 2
    /// @param probs Residue probabilities [in]
    ///
    void SetSeq2ResidueProbs(const TFrequencies& probs) 
    {m_Seq2ResidueProbs = probs;}

    /// Get sequence 2 residue probabilities
    /// @return Residue probabilities for sequence 2
    ///
    const TFrequencies& GetSeq2ResidueProbs(void) const 
    {return m_Seq2ResidueProbs;}

    /// Set maximum calculation time allowed
    /// @param time Maximum calculation time [in]
    ///
    /// Since the gapless calculation is also run during the gapped calculation
    /// (only for a and alpha ungapped parameters) it means that the total
    /// calculation time is equal to time(gapless calculation)+time(gapped
    /// calculation). Typically gapless calculation is quick but sometimes may
    /// take time. The program sets maximum allowed calculation time for gapless
    /// calculation as 0.5*(total maximum time). If the result is not received
    /// during this time then the program reports an error (throw an exception).
    /// After successful ungapped calculation the program calculates during the
    /// remaining available time and uses it for the gapped calculation.     
    /// If the user requests gapless calculation only (m_IsGapped=false) then
    /// the program uses total allowed calculation time for the gapless
    /// calculation. If there is no result during this time then the program
    /// generates an error.
    void SetMaxCalcTime(double time) {m_MaxCalcTime = time;}

    /// Get maximum calculation time allowed
    /// @return Maximum calculation time
    ///
    /// Since the gapless calculation is also run during the gapped calculation
    /// (only for a and alpha ungapped parameters) it means that the total
    /// calculation time is equal to time(gapless calculation)+time(gapped
    /// calculation). Typically gapless calculation is quick but sometimes may
    /// take time. The program sets maximum allowed calculation time for gapless
    /// calculation as 0.5*(total maximum time). If the result is not received
    /// during this time then the program reports an error (throw an exception).
    /// After successful ungapped calculation the program calculates during the
    /// remaining available time and uses it for the gapped calculation.     
    /// If the user requests gapless calculation only (m_IsGapped=false) then
    /// the program uses total allowed calculation time for the gapless
    /// calculation. If there is no result during this time then the program
    /// generates an error.
    double GetMaxCalcTime(void) const {return m_MaxCalcTime;}

    /// Set maximum memory allowed for computation
    /// @param Memory limit [in]
    ///
    void SetMaxCalcMemory(double mem) {m_MaxCalcMemory = mem;} 

    /// Get maximum memory allowed for computation
    /// @return Memory limit
    ///
    double GetMaxCalcMemory(void) const {return m_MaxCalcMemory;}

    /// Get number of residues in utilized alphabet
    /// @return Number of residues
    ///
    Int4 GetNumResidues(void) const {return m_NumResidues;}

    //---- Options Validation ----//

    /// Validate parameter values
    ///
    /// Exception is thrown if options do not pass validation. False is 
    /// returned if there are warnings.
    /// @return True if options passed validation and false if there 
    /// are warnings
    ///
    bool Validate(void);

    /// Check whether there are any error/warning messages
    /// @return True if there are messages, false otherwise
    ///
    bool IsMessage(void) const {return !m_Messages.empty();}

    /// Get error/warning messages
    ///
    /// Warning messages can be generated by the Validate() function if
    /// parameters are within their domains but their values are non-typical or
    /// can result in increased inaccuracies
    /// @return List of error/warning messages
    ///
    const vector<string>& GetMessages(void) const {return m_Messages;}

protected:
    void x_Init(void);
    
    /// Get socre matrix value for i-th and j-th residues
    Int4 x_GetScore(Uint4 i, Uint4 j) const 
    {return (*m_ScoreMatrix).GetScore(i, j);}

private:
    /// Forbid assignment operator
    CGumbelParamsOptions& operator=(const CGumbelParamsOptions&);


protected:

    /// Gap opening penalty
    Int4 m_GapOpening;

    /// Gap extension penalty
    Int4 m_GapExtension;

    /// Desired accuracy for lambda computation
    /// only for gapped aligmment
    double m_LambdaAccuracy;

    /// Desired accuracy for parameter K computation
    /// only for gapped alignment
    double m_KAccuracy;

    /// Gapped/gapless regime, true for gapped, false for gapless.
    /// For gapless regime calculation accuracy for all parameters is
    /// always 1e-06 m_LambdaAccuracy and m_KAccuary are not used.
    bool m_IsGapped;
                                
    /// Scoring matrix
    CConstRef<CGeneralScoreMatrix> m_ScoreMatrix;

    /// Residue frequencies for sequence 1
    TFrequencies m_Seq1ResidueProbs;

    /// Residue frequencies for sequence 2
    TFrequencies m_Seq2ResidueProbs;
    
    /// Number of residues in alphabet
    Int4 m_NumResidues;

    /// Maximum allowed calculation time
    double m_MaxCalcTime;

    /// Maximum allowed calculation memory
    double m_MaxCalcMemory;

    /// Warning messages
    vector<string> m_Messages;
};

/// Class used for creation of sets of input parameters
///
class CGumbelParamsOptionsFactory
{

public:
    /// Creates standard options with score matrix and residue frequenceis for
    /// 20 aa alphabet
    /// @param smat Score matrix name [in]
    /// @return Gumbel params options
    ///
    static CRef<CGumbelParamsOptions> CreateStandard20AAOptions(
                                    CGeneralScoreMatrix::EScoreMatrixName smat
                                    = CGeneralScoreMatrix::eBlosum62);
    

    /// Creates standard options with no score matrix or resiudie frequencies
    /// @return Gumbel params options
    ///
    static CRef<CGumbelParamsOptions> CreateBasicOptions(void);
};


/// Gumbel parameters and estimation errors
///
typedef struct SGumbelParams 
{
    // Gapped parameters
    double lambda;
    double K;
    double C;
    double sigma;
    double alpha_i;
    double alpha_j;
    double ai;
    double aj;

    // Estimation errors for gapped parameters
    double lambda_error;
    double K_error;
    double C_error;
    double sigma_error;
    double alpha_i_error;
    double alpha_j_error;
    double ai_error;
    double aj_error;

    // Gapless parameters
    double gapless_alpha;
    double gapless_a;

    // Estimation errors for gapless parameters
    double gapless_alpha_error;
    double gapless_a_error;    

    Int4 G;

} SGumbelParams;


/// Result of Gumbel parameters calculation along with diagnostic info
///
class CGumbelParamsResult : public CObject
{
public:

    struct SSbsArrays {
        vector<double> lambda_sbs;
        vector<double> K_sbs;
        vector<double> C_sbs;
        vector<double> sigma_sbs;
        vector<double> alpha_i_sbs;
        vector<double> alpha_j_sbs;
        vector<double> ai_sbs;
        vector<double> aj_sbs;
    };

public:
    //---- Constructors ----//
    
    /// Create empty results
    ///
    CGumbelParamsResult(void) {}


    //---- Methods that get the results ----//


    /// Set Gumbel parameters values
    /// @return Gumbel parameters
    ///
    SGumbelParams& SetGumbelParams(void) {return m_GumbelParams;}

    /// Get Gubmel parameters
    /// @return Gumbel parameters
    ///
    const SGumbelParams& GetGumbelParams(void) const {return m_GumbelParams;}

    /// Set Sbs arrays
    /// @return Sbs arrays
    ///
    SSbsArrays& SetSbsArrays(void) {return m_SbsArrays;}

    /// Get Sbs arrays
    /// @return Sbs arrays
    ///
    const SSbsArrays& GetSbsArrays(void) const {return m_SbsArrays;}

    /// Set calculation time
    /// @param time Calculation time [in]
    ///
    void SetCalcTime(double time) {m_CalcTime = time;}

    /// Get calculation time
    /// @return calculation time
    ///
    double GetCalcTime(void) const {return m_CalcTime;}

protected:
    /// Forbid Copy constructor
    CGumbelParamsResult(const CGumbelParamsResult& result);

    /// Forbid assignment operator
    CGumbelParamsResult& operator=(const CGumbelParamsResult&);


protected:

    SGumbelParams m_GumbelParams;
    SSbsArrays m_SbsArrays;

    double m_CalcTime;
};


/// Wrapper for Gumbel parameter calculation
///
class CGumbelParamsCalc : public CObject
{
public:
    //---- Constructors ----//

    /// Create calculation object with given options
    /// @param opts Gumbel params options [in]
    ///
    CGumbelParamsCalc(const CRef<CGumbelParamsOptions>& opts);


    /// Create calculation object with given options and randomization
    /// parameters. The constructor fixes randomization parameters and
    /// makes results predictible. Should be used only for testing.
    /// @param opts Gumbel params options [in]
    /// @param rand_opts Gumbel params randomization parameters
    ///
    CGumbelParamsCalc(const CRef<CGumbelParamsOptions>& opts,
                      const CRef<CGumbelParamsRandDiagnostics>& rand_opts);

    //---- Options ----//

    /// Set randomization parameters. Fixes randomization parameters and
    /// makes results predictible. Should be used only for testing.
    /// @param rand_opts Randomization parameters
    ///
    void SetRandParams(const CRef<CGumbelParamsRandDiagnostics>& rand_opts)
    {m_RandParams = rand_opts;}


    //---- Calculation ----//

    /// Perform Gumbel parameter calculation
    /// @return Gumbel parameters
    ///
    CRef<CGumbelParamsResult> Run(void);


    //---- Getting results ----//

    /// Get randomization parameters
    /// @return Randomization parameters
    ///
    CRef<CGumbelParamsRandDiagnostics> GetRandParams(void) {return m_RandParams;}

    /// Get claculation options
    /// @return Options
    ///
    CConstRef<CGumbelParamsOptions> GetOptions(void) const {return m_Options;}



    /// Get computation result
    /// @return Gumbel parameters and diagnostics
    ///
    CRef<CGumbelParamsResult> GetResult(void) {return m_Result;}

    const SGumbelParams& GetGumbelParams(void) const
    {return m_Result->GetGumbelParams();}
    
protected:
    /// Forbid copy constructor
    CGumbelParamsCalc(const CGumbelParamsCalc&);

    /// Forbid assignment oprator
    CGumbelParamsCalc& operator=(const CGumbelParamsCalc&);


protected:
    CConstRef<CGumbelParamsOptions> m_Options;
    CRef<CGumbelParamsRandDiagnostics> m_RandParams;

    CRef<CGumbelParamsResult> m_Result;

public:
    /// Major version
    static const int kMajorVersion = 2;
    /// Minor version
    static const int kMinorVersion = 0;
    /// Patch version
    static const int kPatchVersion = 0;
};


/// Options that control random values used in internal parts of Gumbel 
/// parameter calculation for gapped aligmment. This class should be used 
/// only for diagnostics. Supplying this class as argument to 
/// CGumbelParamsCalc::Run() yileds deterministic results.
///
class CGumbelParamsRandDiagnostics : public CObject
{
public:

    /// Constructor
    CGumbelParamsRandDiagnostics(void) {}

    /// Set random seed
    /// @param val Random seed [in]
    ///
    void SetRandomSeed(Uint4 val) {m_RandomSeed = val;}

    /// Get random seed
    /// @return Random seed
    ///
    Uint4 GetRandomSeed(void) const {return m_RandomSeed;}

    /// Set first stage preliminary realizations numbers
    /// @return Array of first stage preliminary realizations numbers
    ///
    vector<Int4>& SetFirstStagePrelimReNumbers(void) 
    {return m_FirstStagePrelimReNumbers;}

    /// Get first stage preliminary realizations numbers
    /// @return First stage preliminary realizations numbers
    ///
    const vector<Int4>& GetFirstStagePrelimReNumbers(void) const
    {return m_FirstStagePrelimReNumbers;}
       
    /// Set preliminary realizations numbers
    /// @return Array of preliminary realizations numbers
    ///
    vector<Int4>& SetPrelimReNumbers(void) 
    {return m_PrelimReNumbers;}

    /// Get preliminary realizations numbers
    /// @return Preliminary realizations numbers
    ///
    const vector<Int4>& GetPrelimReNumbers(void) const
    {return m_PrelimReNumbers;}

    /// Set perliminary realizations numbers killing array
    /// @return Array of preliminary realizations numbers killing
    ///
    vector<Int4>& SetPrelimReNumbersKilling(void) 
    {return m_PrelimReNumbersKilling;}

    /// Get perliminary realizations numbers killing array
    /// @return Preliminary realizations numbers killing
    ///
    const vector<Int4>& GetPrelimReNumbersKilling(void) const
    {return m_PrelimReNumbersKilling;}

    /// Set total realizations number
    /// @param num Total realizations number [in]
    ///
    void SetTotalReNumber(Int4 num) {m_TotalReNumber = num;}

    /// Get total realizations number
    /// @return Total realizations number
    ///
    Int4 GetTotalReNumber(void) const {return m_TotalReNumber;}

    /// Set total realizations number killing
    /// @param num Total realizations number killing [in]
    /// 
    void SetTotalReNumberKilling(Int4 num) {m_TotalReNumberKilling = num;}

    /// Get total realizations number killing
    /// @return Total realizations number killing [in]
    /// 
    Int4 GetTotalReNumberKilling(void) const {return m_TotalReNumberKilling;}


private:
    
    /// Random seed
    Uint4 m_RandomSeed;

    /// Frist stage preliminary realizations numbers ALP
    vector<Int4> m_FirstStagePrelimReNumbers;

    // Preliminary realizations numbers ALP
    vector<Int4> m_PrelimReNumbers;

    /// Preliminary realizations numbers killing
    vector<Int4> m_PrelimReNumbersKilling;

    /// Total realizations number ALP
    Int4 m_TotalReNumber;

    // Total realizations number killing
    Int4 m_TotalReNumberKilling;        
};

/// Exception class
class CGumbelParamsException : public CException
{
public:
    enum EErrCode {
        eInvalidOptions,
        eLimitsExceeded, 
        eParamsError, 
        eMemAllocError,
        eUnexpectedError, 
        eRepeatCalc
    };

    virtual const char* GetErrCodeString(void) const {
        return "eInvalid";
    }

    NCBI_EXCEPTION_DEFAULT(CGumbelParamsException, CException);
};

END_SCOPE(blast)
END_NCBI_SCOPE

#endif // ALGO_BLAST_GUMBEL_PARAMS__GUMBEL_PARAMS___HPP


