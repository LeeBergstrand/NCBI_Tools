/*  $Id: test_algo_tree.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
 * Author:  Anatoliy Kuznetov
 *
 * File Description:
 *   TEST for:  NCBI C++ core tree related API
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbireg.hpp>
#include <corelib/ncbi_tree.hpp>
#include <util/bitset/bitset_debug.hpp>
#include <algo/tree/tree_algo.hpp>
#include <util/bitset/ncbi_bitset.hpp>
#include <algorithm>

#include <common/test_assert.h>  /* This header must go last */


// This is to use the ANSI C++ standard templates without the "std::" prefix
// and to use NCBI C++ entities without the "ncbi::" prefix
USING_NCBI_SCOPE;



/// Test application for tree algorithms
///
/// @internal
///
class CAlgoTreeTestApplication : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};


void CAlgoTreeTestApplication::Init(void)
{
    // Set err.-posting and tracing to maximum
    SetDiagTrace(eDT_Enable);
    SetDiagPostFlag(eDPF_All);
    SetDiagPostLevel(eDiag_Info);
}

ETreeTraverseCode TestFunctor1(CTreeNode<int>& tr, int delta)
{
    cout << tr.GetValue() << " :" << delta << endl;
    return eTreeTraverse;
}

ETreeTraverseCode TestFunctor2(CTreeNode<int>& tr)
{
  cout << tr.GetValue() << endl;
  return eTreeTraverse;
}

bool TestFunctor3(int a, int b) 
{
  return (a == b);
}

static string s_IntToStr(int i)
{
    return NStr::IntToString(i);
}

static void s_TEST_TreeOperations()
{
  
  typedef CTreeNode<int> TTree;
  
  TTree* orig = new TTree(0);
  TTree* orig10 = orig->AddNode(10);
  TTree* orig11 = orig->AddNode(11);
  orig10->AddNode(20);
  orig10->AddNode(21);
  orig11->AddNode(22);
  orig11->AddNode(23);
  
  TTree* corr = new TTree(11);
  corr->AddNode(22);
  corr->AddNode(23);
  TTree* corr0 = corr->AddNode(0);
  TTree* corr10 = corr0->AddNode(10);
  corr10->AddNode(20);
  corr10->AddNode(21);

  TreeReRoot(*orig10);

  cout << "After rerooting original tree by node 10, " << endl;
  cout << "the original tree and correct tree are now ";
  if(TreeCompare(*orig10, *corr, TestFunctor3)) {
    cout << "the same." << endl;
  }
  else {
    cout << "different." << endl;
  }
  cout << endl;

  TreePrint(cout, *orig10, s_IntToStr, false);
  TreePrint(cout, *corr, s_IntToStr, false);
  cout << endl;
  /*
  TTree* corr2 = new TTree(11);
  corr2->AddNode(22);
  corr2->AddNode(23);
  corr2->AddNode(0);
 
  TTree* t = orig->DetachNode(orig10);

  cout << "After removing node 10 from this tree, " << endl;
  cout << "the original tree and correct tree are now ";
  if(TreeCompare(*orig11, *corr2, TestFunctor3)) {
    cout << "the same." << endl;
  }
  else {
    cout << "different." << endl;
  }
  cout << endl;
   
  TreePrint(cout, *orig11, s_IntToStr, false);
  TreePrint(cout, *t, s_IntToStr, false);
  TreePrint(cout, *corr2, s_IntToStr, false);
  cout << endl;
  */
    delete orig->GetRoot();
    delete corr->GetRoot();
}

static void s_TEST_Tree()
{
    typedef CTreeNode<int>  TTree;
    
    TTree* tr = new TTree(0);
    TTree* tr10 = tr->AddNode(10);
    tr->AddNode(11);
    tr10->AddNode(20);
    tr10->AddNode(21);
   
    TTree* sr = new TTree(0);
    sr->AddNode(10);
    sr->AddNode(11);

    TTree* ur = new TTree(0);
    ur->AddNode(10);
    ur->AddNode(11);
    ur->AddNode(20);
    ur->AddNode(21);
    

//    TreePrint(cout, *tr, (IntConvType) NStr::IntToString);
//    TreeReRoot(*tr10);
//    TreePrint(cout, *tr10, (IntConvType) NStr::IntToString);

    cout << "Testing Breadth First Traversal" << endl;
    TreeBreadthFirstTraverse(*tr, TestFunctor2);
    cout << endl;

    cout << "Testing Depth First Traversal" << endl;
    TreeDepthFirstTraverse(*tr, TestFunctor1);
    cout << endl;

    cout << "Testing Tree Comparison" << endl;
    cout << "tr and tr10 are ";
    if(!TreeCompare(*tr, *tr10, TestFunctor3)) cout << "not ";
    cout << "the same." << endl;
    cout << "tr and tr are ";
    if(!TreeCompare(*tr, *tr, TestFunctor3)) cout << "not ";
    cout << "the same." << endl;
    cout << "tr and sr are ";
    if(!TreeCompare(*tr, *sr, TestFunctor3)) cout << "not ";
    cout << "the same." << endl;
    cout << "tr and ur are ";
    if(!TreeCompare(*tr, *ur, TestFunctor3)) cout << "not ";
    cout << "the same." << endl;
    cout << "sr and ur are ";
    if(!TreeCompare(*sr, *ur, TestFunctor3)) cout << "not ";
    cout << "the same." << endl;
    cout << endl;

    {{
    unsigned int cnt;
    TTree::TNodeList_CI it = tr->SubNodeBegin();
    TTree::TNodeList_CI it_end = tr->SubNodeEnd();
    
    for (cnt = 0; it != it_end; ++it, ++cnt) {
        const TTree* t = *it;
        int v = t->GetValue();
        assert(v == 10 || v == 11);
    }
    assert(cnt == 2);
    }}
    
    {{
    TTree* tr2 = new TTree(*tr);
    unsigned int cnt;
    TTree::TNodeList_CI it = tr2->SubNodeBegin();
    TTree::TNodeList_CI it_end = tr2->SubNodeEnd();
    
    for (cnt = 0; it != it_end; ++it, ++cnt) {
        const TTree* t = *it;
        int v = t->GetValue();
        assert(v == 10 || v == 11);
    }
    assert(cnt == 2);
    delete tr2;
    }}
    
    
    {{
    TTree::TNodeList_I it = tr->SubNodeBegin();
    TTree::TNodeList_I it_end = tr->SubNodeEnd();
    
    for (; it != it_end; ++it) {
        TTree* t = *it;
        int v = t->GetValue();
        if (v == 10)
        {
            tr->RemoveNode(t);
            break;
        }
    }
    }}

    TreeDepthFirstTraverse(*tr, TestFunctor1);
    cout << endl;

    {{
    unsigned int cnt;
    TTree::TNodeList_CI it = tr->SubNodeBegin();
    TTree::TNodeList_CI it_end = tr->SubNodeEnd();
    
    for (cnt = 0; it != it_end; ++it, ++cnt) {
        const TTree* t = *it;
        int v = t->GetValue();
        assert(v == 11);
    }
    assert(cnt == 1);
    }}
    
    delete tr;

    TTree* str = tr = new TTree(0);
    
    //
    // 0 - 2 
    //       - 4
    //   - 3 
    //       - 5
    //       - 6
    //

    TTree* tr4 = tr->AddNode(2)->AddNode(4);
    tr = tr->AddNode(3);
    TTree* tr5 = tr->AddNode(5);
    TTree* tr6 = tr->AddNode(6);

    cout << "Test Tree: " << endl;

    TreeDepthFirstTraverse(*str, TestFunctor1);
    cout << endl;

    vector<const TTree*> trace_vec;
    TreeTraceToRoot(*tr6, trace_vec);

    assert(trace_vec.size() == 3);

    {{
    cout << "Trace to root: ";

    ITERATE(vector<const TTree*>, it, trace_vec) {
        cout << (*it)->GetValue() << "; ";
    }

    cout << endl;

    }}

    const TTree* parent_node = TreeFindCommonParent(*tr4, *tr6);

    assert(tr4->IsParent(*parent_node));
    assert(tr6->IsParent(*parent_node));
    assert(!tr4->IsParent(*tr6));


    assert(parent_node);

    cout << "parent: " << parent_node->GetValue() << endl;

    assert(parent_node->GetValue() == 0);

    parent_node = TreeFindCommonParent(*tr5, *tr6);
    assert(parent_node);
    assert(parent_node->GetValue() == 3);


    TreePrint(cout, *str, s_IntToStr);
    cout << endl;

    TreeReRoot(*tr5);
    TreePrint(cout, *tr5, s_IntToStr);

    tr5->MoveSubnodes(str);
    TreePrint(cout, *tr5, s_IntToStr);

    delete tr5;
    delete ur;
    delete sr;
}

struct IdValue
{
    int id;
    
    IdValue() : id(0) {}
    IdValue(int v) : id(v) {}

    operator int() const { return id; }
    int GetId() const { return id; }
};

static string s_IdValueToStr(const IdValue& idv)
{
    return NStr::IntToString(idv.id);
}

static void s_TEST_IdTreeOperations()
{
    cout << "--------------------- s_TEST_IdTreeOperations " << endl;

    typedef CTreeNode<IdValue> TTree;

    TTree* tr = new TTree(0);

    TTree* tr10 = tr->AddNode(10);
    TTree* tr11 = tr->AddNode(11);
    TTree* tr110 = tr10->AddNode(110);
    TTree* tr1100 = tr110->AddNode(1100);

    TreePrint(cout, *tr, s_IdValueToStr);

    bm::bvector<> bv;
    TreeMakeSubNodesSet(*tr, bv.inserter());
    assert(bv.count() == 2);
    assert(bv[10]);
    assert(bv[11]);

    typedef vector<TTree*> TNodeList;
    TNodeList node_list;
    node_list.push_back(tr10);
    node_list.push_back(tr11);
    node_list.push_back(tr110);
    node_list.push_back(tr1100);

    TNodeList res_node_list;

    CTreeNonRedundantSet<TTree, bm::bvector<>, TNodeList> nr_func;
    nr_func(node_list, res_node_list);

    cout << "Non-redundant set:" << endl;
    ITERATE(TNodeList, it, res_node_list) {
        cout << (*it)->GetValue().GetId() << "; ";
    }
    cout << endl;
    assert(res_node_list.size() == 2);


    res_node_list.clear();
    node_list.clear();

    node_list.push_back(tr110);
    node_list.push_back(tr1100);

    CTreeMinimalSet<TTree, bm::bvector<>, TNodeList> min_func;
    min_func(node_list, res_node_list);

    
    cout << "Minimal set:" << endl;
    ITERATE(TNodeList, it, res_node_list) {
        cout << (*it)->GetValue().GetId() << "; ";
    }
    cout << endl;
    cout << "-----" << endl;
    assert(res_node_list.size() == 1);


    res_node_list.clear();
    node_list.clear();


    node_list.push_back(tr110);
    node_list.push_back(tr1100);
    node_list.push_back(tr11);

    min_func(node_list, res_node_list);

    
    cout << "Minimal set:" << endl;
    ITERATE(TNodeList, it, res_node_list) {
        cout << (*it)->GetValue().GetId() << "; ";
    }
    cout << endl;
    cout << "-----" << endl;
    assert(res_node_list.size() == 1);


    res_node_list.clear();
    node_list.clear();


    
    TNodeList node_list_a;
    TNodeList node_list_b;
    TNodeList node_list_c;

    node_list_a.push_back(tr10);
    node_list_a.push_back(tr11);
    node_list_a.push_back(tr110);
    node_list_a.push_back(tr1100);
    
    node_list_b.push_back(tr10);
    node_list_b.push_back(tr11);
    node_list_b.push_back(tr110);

    CTreeNodesAnd<TTree, bm::bvector<>, TNodeList> and_func;
    and_func(node_list_a, node_list_b, node_list_c);

    ITERATE(TNodeList, it, node_list_c) {
        cout << (*it)->GetValue().GetId() << "; ";
    }
    cout << endl;
    assert(node_list_c.size() == 3);


    node_list_c.clear();

    CTreeNodesOr<TTree, bm::bvector<>, TNodeList> or_func;
    or_func(node_list_a, node_list_b, node_list_c);

    ITERATE(TNodeList, it, node_list_c) {
        cout << (*it)->GetValue().GetId() << "; ";
    }
    cout << endl;
    assert(node_list_c.size() == 4);


    delete tr;

    cout << "--------------------- s_TEST_IdTreeOperations ok" << endl;
}

static void s_TEST_IdTree()
{
    typedef CTreePair<int, int>::TTreePair     TTreePair;
    typedef CTreePair<int, int>::TPairTreeNode TTree;

    TTree* tr = new TTree(TTreePair(0, 0));
    
    tr->AddNode(TTreePair(1, 10));
    tr->AddNode(TTreePair(100, 110));
    TTree* tr2 = tr->AddNode(TTreePair(2, 20));
    tr2->AddNode(TTreePair(20, 21));    
    TTree* tr3 =tr2->AddNode(TTreePair(22, 22));
    tr3->AddNode(TTreePair(222, 222));

    {{
    const TTree* tnd = tr->FindSubNode(100);
    assert(tnd);
    assert(tnd->GetValue().id == 100);
    assert(tnd->GetValue().value == 110);
    const TTree& tv = tnd->GetValue();
    assert(tv.GetValue().value == 110);

    }}

    {{
    list<int> npath;
    npath.push_back(2);
    npath.push_back(22);

    TTree::TConstNodeList res;
    tr->FindNodes(npath, &res);
    assert(!res.empty());
    TTree::TConstNodeList::const_iterator it = res.begin();
    const TTree* ftr = *it;

    assert(ftr->GetValue().value == 22);
    }}

    {{
    list<int> npath;
    npath.push_back(2);
    npath.push_back(32);

    TTree::TConstNodeList res;
    tr->FindNodes(npath, &res);
    assert(res.empty());
    }}

    {{
    list<int> npath;
    npath.push_back(2);
    npath.push_back(22);
    npath.push_back(222);
    npath.push_back(100);

    const TTree* node = PairTreeTraceNode(*tr, npath);
    assert(node);
    cout << node->GetValue().id << " " << node->GetValue().value << endl;
    assert(node->GetValue().value == 110);


    node = PairTreeBackTraceNode(*tr3, 1);
    assert(node);
    cout << node->GetValue().id << " " << node->GetValue().value << endl;
    assert(node->GetValue().value == 10);
    }}

    {{
    list<int> npath;
    npath.push_back(2);
    npath.push_back(22);
    npath.push_back(222);

    const TTree* node = PairTreeTraceNode(*tr, npath);
    assert(node);
    cout << node->GetValue().id << " " << node->GetValue().value << endl;
    assert(node->GetValue().value == 222);
    }}

    delete tr;
}



int CAlgoTreeTestApplication::Run(void)
{


    //        CExceptionReporterStream reporter(cerr);
    //        CExceptionReporter::SetDefault(&reporter);
    //        CExceptionReporter::EnableDefault(false);
    //        CExceptionReporter::EnableDefault(true);
    //        CExceptionReporter::SetDefault(0);
    
    /*      
    CExceptionReporter::EnableDefault(true);
    cerr << endl;
    NCBI_REPORT_EXCEPTION(
    "****** default reporter (stream) ******",e);

    CExceptionReporter::SetDefault(0);
    cerr << endl;
    NCBI_REPORT_EXCEPTION(
    "****** default reporter (diag) ******",e);
    */
    
    s_TEST_Tree();

    s_TEST_TreeOperations();

    s_TEST_IdTree();

    s_TEST_IdTreeOperations();

    return 0;
}

  
/////////////////////////////////
// APPLICATION OBJECT and MAIN
//

int main(int argc, const char* argv[] /*, const char* envp[]*/)
{
    CAlgoTreeTestApplication theTestApplication;
    return theTestApplication.AppMain(argc, argv, 0 /*envp*/, eDS_ToMemory);
}
