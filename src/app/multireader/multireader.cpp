/*  $Id: multireader.cpp 387081 2013-01-25 15:37:41Z ludwigf $
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
* Author:  Frank Ludwig, NCBI
*
* File Description:
*   Reader for selected data file formats
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbistl.hpp>
#include <corelib/ncbi_system.hpp>
#include <util/format_guess.hpp>
#include <util/line_reader.hpp>

#include <serial/iterator.hpp>
#include <serial/objistr.hpp>
#include <serial/objostr.hpp>
#include <serial/objostrasn.hpp>
#include <serial/serial.hpp>

#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <objects/seq/Seq_annot.hpp>
#include <objects/seqset/Seq_entry.hpp>

#include <objtools/readers/reader_exception.hpp>
#include <objtools/readers/line_error.hpp>
#include <objtools/readers/error_container.hpp>
#include <objtools/readers/idmapper.hpp>
#include <objtools/readers/reader_base.hpp>
#include <objtools/readers/bed_reader.hpp>
#include <objtools/readers/vcf_reader.hpp>
#include <objtools/readers/wiggle_reader.hpp>
#include <objtools/readers/gff2_reader.hpp>
#include <objtools/readers/gff3_reader.hpp>
#include <objtools/readers/gtf_reader.hpp>
#include <objtools/readers/gvf_reader.hpp>
#include <objtools/readers/aln_reader.hpp>
#include <objtools/readers/agp_read.hpp>

#include <algo/phy_tree/phy_node.hpp>
#include <algo/phy_tree/dist_methods.hpp>
#include <objects/biotree/BioTreeContainer.hpp>

USING_NCBI_SCOPE;
USING_SCOPE(objects);

//  ============================================================================
void
DumpMemory(
    const string& prefix)
//  ============================================================================
{
    Uint8 totalMemory = GetPhysicalMemorySize();
    size_t usedMemory; size_t residentMemory; size_t sharedMemory;
    if (!GetMemoryUsage(&usedMemory, &residentMemory, &sharedMemory)) {
        cerr << "Unable to get memory counts!" << endl;
    }
    else {
        cerr << prefix
            << "Total:" << totalMemory 
            << " Used:" << usedMemory << "(" 
                << (100*usedMemory)/totalMemory <<"%)" 
            << " Resident:" << residentMemory << "(" 
                << int((100.0*residentMemory)/totalMemory) <<"%)" 
            << endl;
    }
}
  
//  ============================================================================
class CMultiReaderApp
//  ============================================================================
     : public CNcbiApplication
{
public:
    CMultiReaderApp(): m_pErrors( 0 ) {};
    
protected:
       
private:
    virtual void Init(void);
    virtual int  Run(void);
    
    void xProcessDefault(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessWiggle(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessWiggleRaw(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessBed(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessBedRaw(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessGtf(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessVcf(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessNewick(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessGff3(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessGff2(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessGvf(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessAlignment(const CArgs&, CNcbiIstream&, CNcbiOstream&);
    void xProcessAgp(const CArgs&, CNcbiIstream&, CNcbiOstream&);

    void xSetFormat(const CArgs&, CNcbiIstream&);
    void xSetFlags(const CArgs&, CNcbiIstream&);
    void xSetMapper(const CArgs&);
    void xSetErrorContainer(const CArgs&);
            
    void xWriteObject(CSerialObject&, CNcbiOstream& );
    void xDumpErrors(CNcbiOstream& );

    CFormatGuess::EFormat m_uFormat;
    bool m_bCheckOnly;
    bool m_bDumpStats;
    int  m_iFlags;
    string m_AnnotName;
    string m_AnnotTitle;

    auto_ptr<CIdMapper> m_pMapper;
    CRef<CErrorContainerBase> m_pErrors;
};

//  ============================================================================
class CErrorContainerCustom:
//  ============================================================================
    public CErrorContainerBase
{
public:
    CErrorContainerCustom(
        int iMaxCount,
        int iMaxLevel ): m_iMaxCount( iMaxCount ), m_iMaxLevel( iMaxLevel ) {};
    ~CErrorContainerCustom() {};
    
    bool
    PutError(
        const ILineError& err ) 
    {
        m_Errors.push_back( 
            CLineError( err.Problem(), err.Severity(), err.SeqId(), err.Line(), 
                err.FeatureName(), err.QualifierName(), err.QualifierValue() ) );
        return (err.Severity() <= m_iMaxLevel) && (m_Errors.size() < m_iMaxCount);
    };
    
protected:
    size_t m_iMaxCount;
    int m_iMaxLevel;    
};    
        
//  ----------------------------------------------------------------------------
void CMultiReaderApp::Init(void)
//  ----------------------------------------------------------------------------
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);

    arg_desc->SetUsageContext
        (GetArguments().GetProgramBasename(),
         "C++ multi format file reader");

    //
    //  input / output:
    //        
    arg_desc->AddKey(
        "input", 
        "File_In",
        "Input filename",
        CArgDescriptions::eInputFile);

    arg_desc->AddDefaultKey(
        "output", 
        "File_Out",
        "Output filename",
        CArgDescriptions::eOutputFile, "-"); 

    arg_desc->AddDefaultKey(
        "format", 
        "STRING",
        "Input file format",
        CArgDescriptions::eString, 
        "guess");
    arg_desc->SetConstraint(
        "format", 
        &(*new CArgAllow_Strings, 
            "bed", 
            "microarray", "bed15", 
            "wig", "wiggle", 
            "gtf", "gff3", "gff2",
            "gvf",
            "agp",
            "newick", "tree", "tre",
            "vcf",
            "aln", "align",
            "hgvs",
            "guess") );

    arg_desc->AddDefaultKey(
        "flags",
        "INTEGER",
        "Additional flags passed to the reader",
        CArgDescriptions::eString,
        "0" );

    arg_desc->AddDefaultKey(
        "name", 
        "STRING",
        "Name for annotation",
        CArgDescriptions::eString, 
        "");
    arg_desc->AddDefaultKey(
        "title", 
        "STRING",
        "Title for annotation",
        CArgDescriptions::eString, 
        "");

    //
    //  ID mapping:
    //
    arg_desc->AddDefaultKey(
        "mapfile",
        "File_In",
        "IdMapper config filename",
        CArgDescriptions::eString, "" );

    arg_desc->AddDefaultKey(
        "genome", 
        "STRING",
        "UCSC build number",
        CArgDescriptions::eString, 
        "" );
        
    //
    //  Error policy:
    //        
    arg_desc->AddFlag(
        "dumpstats",
        "write record counts to stderr",
        true );
        
    arg_desc->AddFlag(
        "checkonly",
        "check for errors only",
        true );
        
    arg_desc->AddFlag(
        "noerrors",
        "suppress error display",
        true );
        
    arg_desc->AddFlag(
        "lenient",
        "accept all input format errors",
        true );
        
    arg_desc->AddFlag(
        "strict",
        "accept no input format errors",
        true );
        
    arg_desc->AddDefaultKey(
        "max-error-count", 
        "INTEGER",
        "Maximum permissible error count",
        CArgDescriptions::eInteger,
        "-1" );
        
    arg_desc->AddDefaultKey(
        "max-error-level", 
        "STRING",
        "Maximum permissible error level",
        CArgDescriptions::eString,
        "warning" );
        
    arg_desc->SetConstraint(
        "max-error-level", 
        &(*new CArgAllow_Strings, 
            "info", "warning", "error" ) );

    //
    //  bed and gff reader specific arguments:
    //
    arg_desc->AddFlag(
        "all-ids-as-local",
        "turn all ids into local ids",
        true );
        
    arg_desc->AddFlag(
        "numeric-ids-as-local",
        "turn integer ids into local ids",
        true );
        
    //
    //  wiggle reader specific arguments:
    //
    arg_desc->AddFlag(
        "join-same",
        "join abutting intervals",
        true );
        
    arg_desc->AddFlag(
        "as-byte",
        "generate byte compressed data",
        true );
    
    arg_desc->AddFlag(
        "as-graph",
        "generate graph object",
        true );

    arg_desc->AddFlag(
        "raw",
        "iteratively return raw track data",
        true );

    //
    //  gff reader specific arguments:
    //
    arg_desc->AddFlag( // no longer used, retained for backward compatibility
        "new-code",
        "use new gff3 reader implementation",
        true );    
    arg_desc->AddFlag(
        "old-code",
        "use old gff3 reader implementation",
        true );    

    //
    //  alignment reader specific arguments:
    //
    arg_desc->AddDefaultKey(
        "aln-gapchar", 
        "STRING",
        "Alignment gap character",
        CArgDescriptions::eString, 
        "-");

    arg_desc->AddDefaultKey(
        "aln-alphabet", 
        "STRING",
        "Alignment alphabet",
        CArgDescriptions::eString, 
        "nuc");
    arg_desc->SetConstraint(
        "aln-alphabet", 
        &(*new CArgAllow_Strings, 
            "nuc", 
            "prot") );    

    arg_desc->AddFlag(
        "force-local-ids",
        "treat all IDs as local IDs",
        true);

    SetupArgDescriptions(arg_desc.release());
}

//  ----------------------------------------------------------------------------
int 
CMultiReaderApp::Run(void)
//  ----------------------------------------------------------------------------
{   
    const CArgs& args = GetArgs();
    CNcbiIstream& istr = args["input"].AsInputFile();
    CNcbiOstream& ostr = args["output"].AsOutputFile();

    xSetFormat(args ,istr);    
    xSetFlags(args, istr);
    xSetMapper(args);
    xSetErrorContainer(args);

    CRef< CSerialObject> object;
    vector< CRef< CSeq_annot > > annots;
    switch( m_uFormat ) {
        default: 
            xProcessDefault(args, istr, ostr);   
            break;
        case CFormatGuess::eWiggle:
            if (m_iFlags & CReaderBase::fAsRaw) {
                xProcessWiggleRaw(args, istr, ostr);
            }
            else {
                xProcessWiggle(args, istr, ostr);
            }
            break;
        case CFormatGuess::eBed:
            if (m_iFlags & CReaderBase::fAsRaw) {
                xProcessBedRaw(args, istr, ostr);
            }
            else {
                xProcessBed(args, istr, ostr);
            }
            break;
        case CFormatGuess::eGtf:
        case CFormatGuess::eGtf_POISENED:
            xProcessGtf(args, istr, ostr);
            break;
        case CFormatGuess::eVcf:
            xProcessVcf(args, istr, ostr);
            break;
        case CFormatGuess::eNewick:
            xProcessNewick(args, istr, ostr);
            break;
        case CFormatGuess::eGff3:
            xProcessGff3(args, istr, ostr);
            break;
        case CFormatGuess::eGff2:
            xProcessGff2(args, istr, ostr);
            break;
        case CFormatGuess::eGvf:
            xProcessGvf(args, istr, ostr);
            break;
        case CFormatGuess::eAgp:
            xProcessAgp(args, istr, ostr);
            break;
        case CFormatGuess::eAlignment:
            xProcessAlignment(args, istr, ostr);
            break;
    }

    xDumpErrors( cerr );
    return 0;
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessDefault(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    auto_ptr<CReaderBase> pReader(CReaderBase::GetReader(m_uFormat, m_iFlags));
    if (!pReader.get()) {
        NCBI_THROW2(CObjReaderParseException, eFormat,
            "File format not supported", 0);
    }
    CRef<CSerialObject> object = pReader->ReadObject(istr, m_pErrors);
    xWriteObject(*object, ostr);
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessWiggle(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;
    
    CWiggleReader reader(m_iFlags);
    CStreamLineReader lr(istr);
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessWiggleRaw(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    CWiggleReader reader(m_iFlags);
    CStreamLineReader lr(istr);
    CRawWiggleTrack raw;
    while (reader.ReadTrackData(lr, raw)) {
        raw.Dump(cerr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessBed(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    //  Use ReadSeqAnnot() over ReadSeqAnnots() to keep memory footprint down.
    CBedReader reader(m_iFlags);
    CStreamLineReader lr(istr);
    CRef<CSeq_annot> pAnnot = reader.ReadSeqAnnot(lr, m_pErrors);
    while(pAnnot) {
        xWriteObject(*pAnnot, ostr);
        pAnnot.Reset();
        pAnnot = reader.ReadSeqAnnot(lr, m_pErrors);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessBedRaw(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    CBedReader reader(m_iFlags);
    CStreamLineReader lr(istr);
    CRawBedTrack raw;
    while (reader.ReadTrackData(lr, raw)) {
        raw.Dump(cerr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessGtf(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;
    
    if (args["format"].AsString() == "gff2") { // process as plain GFF2
        return xProcessGff2(args, istr, ostr);
    }
    CGtfReader reader(m_iFlags, m_AnnotName, m_AnnotTitle);
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessGff3(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;
    
    if (args["format"].AsString() == "gff2") { // process as plain GFF2
        return xProcessGff2(args, istr, ostr);
    }
    CGff3Reader reader(m_iFlags, m_AnnotName, m_AnnotTitle);
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessGff2(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;
    
    CGff2Reader reader(m_iFlags, m_AnnotName, m_AnnotTitle);
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessGvf(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;
    
    if (args["format"].AsString() == "gff2") { // process as plain GFF2
        return xProcessGff2(args, istr, ostr);
    }
    if (args["format"].AsString() == "gff3") { // process as plain GFF2
        return xProcessGff3(args, istr, ostr);
    }
    CGvfReader reader(m_iFlags, m_AnnotName, m_AnnotTitle);
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessVcf(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_annot> > ANNOTS;
    ANNOTS annots;

    CVcfReader reader( m_iFlags );
    reader.ReadSeqAnnots(annots, istr, m_pErrors);
    for (ANNOTS::iterator cit = annots.begin(); cit != annots.end(); ++cit){
        xWriteObject(**cit, ostr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessNewick(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    string strTree;
    char c = istr.get();
    while (!istr.eof()) {
        strTree += c;
        if (c == ';') {
            CNcbiIstrstream istrstr(strTree.c_str());
            auto_ptr<TPhyTreeNode>  pTree(ReadNewickTree(istrstr) );
            CRef<CBioTreeContainer> btc = MakeBioTreeContainer(pTree.get());
            xWriteObject(*btc, ostr);
            strTree.clear();
        }
        c = istr.get();
    }
}


//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessAgp(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    typedef vector<CRef<CSeq_entry> > TEntries;
    TEntries entries;
    AgpRead(istr, entries);
    NON_CONST_ITERATE (TEntries, it, entries) {
        xWriteObject(**it, ostr);
    }
}


//  ----------------------------------------------------------------------------
void CMultiReaderApp::xProcessAlignment(
    const CArgs& args,
    CNcbiIstream& istr,
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    CAlnReader reader(istr);
    reader.SetAllGap(args["aln-gapchar"].AsString());
    reader.SetAlphabet(CAlnReader::eAlpha_Nucleotide);
    if (args["aln-alphabet"].AsString() == "prot") {
        reader.SetAlphabet(CAlnReader::eAlpha_Protein);
    }
    reader.Read(false, args["force-local-ids"]);
    CRef<CSeq_align> pAlign = reader.GetSeqAlign();
    xWriteObject(*pAlign, ostr);
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xSetFormat(
    const CArgs& args,
    CNcbiIstream& istr )
//  ----------------------------------------------------------------------------
{
    m_uFormat = CFormatGuess::eUnknown;    
    string format = args["format"].AsString();
    const string& strProgramName = GetProgramDisplayName();
    
    if (NStr::StartsWith(strProgramName, "wig") || format == "wig" ||
        format == "wiggle") {
        m_uFormat = CFormatGuess::eWiggle;
    }
    if (NStr::StartsWith(strProgramName, "bed") || format == "bed") {
        m_uFormat = CFormatGuess::eBed;
    }
    if (NStr::StartsWith(strProgramName, "b15") || format == "bed15" ||
        format == "microarray") {
        m_uFormat = CFormatGuess::eBed15;
    }
    if (NStr::StartsWith(strProgramName, "gtf") || format == "gtf") {
        m_uFormat = CFormatGuess::eGtf;
    }
    if (NStr::StartsWith(strProgramName, "gff") || 
        format == "gff3" || format == "gff2") {
        m_uFormat = CFormatGuess::eGff3;
    }
    if (NStr::StartsWith(strProgramName, "agp")) {
        m_uFormat = CFormatGuess::eAgp;
    }

    if (NStr::StartsWith(strProgramName, "newick") || 
        format == "newick" || format == "tree" || format == "tre") {
        m_uFormat = CFormatGuess::eNewick;
    }
    if (NStr::StartsWith(strProgramName, "gvf") || format == "gvf") {
        m_uFormat = CFormatGuess::eGtf;
    }
    if (NStr::StartsWith(strProgramName, "aln") || format == "align" ||
        format == "aln") {
        m_uFormat = CFormatGuess::eAlignment;
    }
    if (NStr::StartsWith(strProgramName, "hgvs") || 
        format == "hgvs") {
        m_uFormat = CFormatGuess::eHgvs;
    }
    if (m_uFormat == CFormatGuess::eUnknown) {
        m_uFormat = CFormatGuess::Format(istr);
    }
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xSetFlags(
    const CArgs& args,
    CNcbiIstream& istr)
//  ----------------------------------------------------------------------------
{
    if (m_uFormat == CFormatGuess::eUnknown) {
        xSetFormat(args, istr);
    }
    m_iFlags = NStr::StringToInt( 
        args["flags"].AsString(), NStr::fConvErr_NoThrow, 16 );
        
    switch( m_uFormat ) {
    
    case CFormatGuess::eWiggle:
        if ( args["join-same"] ) {
            m_iFlags |= CWiggleReader::fJoinSame;
        }
        //by default now. But still tolerate if explicitly specified.
        m_iFlags |= CWiggleReader::fAsByte;

        if ( args["as-graph"] ) {
            m_iFlags |= CWiggleReader::fAsGraph;
        }

        if ( args["raw"] ) {
            m_iFlags |= CReaderBase::fAsRaw;
        }
        break;
    
    case CFormatGuess::eBed:
        if ( args["all-ids-as-local"] ) {
            m_iFlags |= CBedReader::fAllIdsAsLocal;
        }
        if ( args["numeric-ids-as-local"] ) {
            m_iFlags |= CBedReader::fNumericIdsAsLocal;
        }
        if ( args["raw"] ) {
            m_iFlags |= CReaderBase::fAsRaw;
        }
        break;
       
    case CFormatGuess::eGtf:
        if ( args["all-ids-as-local"] ) {
            m_iFlags |= CGFFReader::fAllIdsAsLocal;
        }
        if ( args["numeric-ids-as-local"] ) {
            m_iFlags |= CGFFReader::fNumericIdsAsLocal;
        }
            
    default:
        break;
    }
    m_AnnotName = args["name"].AsString();
    m_AnnotTitle = args["title"].AsString();
    m_bCheckOnly = args["checkonly"];
}

//  ----------------------------------------------------------------------------
void CMultiReaderApp::xWriteObject(
    CSerialObject& object,                  // potentially modified by mapper
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    if (m_pMapper.get()) {
        m_pMapper->MapObject(object);
    }
    if (m_bCheckOnly) {
        return;
    }
    ostr << MSerial_AsnText << object;
    ostr.flush();
}
        
//  ----------------------------------------------------------------------------
void
CMultiReaderApp::xSetMapper(
    const CArgs& args)
//  ----------------------------------------------------------------------------
{
    string strBuild = args["genome"].AsString();
    string strMapFile = args["mapfile"].AsString();
    
    if (strBuild.empty() && strMapFile.empty()) {
        return;
    }
    if (!strMapFile.empty()) {
        CNcbiIfstream* pMapFile = new CNcbiIfstream(strMapFile.c_str());
        m_pMapper.reset(
            new CIdMapperConfig(*pMapFile, strBuild, false, m_pErrors));
    }
    else {
        m_pMapper.reset(new CIdMapperBuiltin(strBuild, false, m_pErrors));
    }
}        

//  ----------------------------------------------------------------------------
void
CMultiReaderApp::xSetErrorContainer(
    const CArgs& args )
//  ----------------------------------------------------------------------------
{
    //
    //  By default, allow all errors up to the level of "warning" but nothing
    //  more serious. -strict trumps everything else, -lenient is the second
    //  strongest. In the absence of -strict and -lenient, -max-error-count and
    //  -max-error-level become additive, i.e. both are enforced.
    //
    if ( args["noerrors"] ) {   // not using error policy at all
        m_pErrors = 0;
        return;
    }
    if ( args["strict"] ) {
        m_pErrors = new CErrorContainerStrict;
        return;
    }
    if ( args["lenient"] ) {
        m_pErrors = new CErrorContainerLenient;
        return;
    }
    
    int iMaxErrorCount = args["max-error-count"].AsInteger();
    int iMaxErrorLevel = eDiag_Error;
    string strMaxErrorLevel = args["max-error-level"].AsString();
    if ( strMaxErrorLevel == "info" ) {
        iMaxErrorLevel = eDiag_Info;
    }
    else if ( strMaxErrorLevel == "error" ) {
        iMaxErrorLevel = eDiag_Error;
    }
    
    if ( iMaxErrorCount == -1 ) {
        m_pErrors.Reset(new CErrorContainerLevel(iMaxErrorLevel));
        return;
    }
    m_pErrors.Reset(new CErrorContainerCustom(iMaxErrorCount, iMaxErrorLevel));
}
    
//  ----------------------------------------------------------------------------
void CMultiReaderApp::xDumpErrors(
    CNcbiOstream& ostr)
//  ----------------------------------------------------------------------------
{
    if (m_pErrors) {
        m_pErrors->Dump(ostr);
    }
}

//  ----------------------------------------------------------------------------
int main(int argc, const char* argv[])
//  ----------------------------------------------------------------------------
{
    // Execute main application function
    return CMultiReaderApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
