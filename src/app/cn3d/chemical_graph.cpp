/*  $Id: chemical_graph.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
*      Classes to hold the graph of chemical bonds
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include <objects/mmdb1/Biostruc_residue_graph_set.hpp>
#include <objects/mmdb1/Biostruc_id.hpp>
#include <objects/general/Dbtag.hpp>
#include <objects/mmdb3/Biostruc_feature_set_descr.hpp>
#include <objects/mmdb3/Biostruc_feature.hpp>
#include <objects/mmdb3/Chem_graph_pntrs.hpp>
#include <objects/mmdb3/Residue_pntrs.hpp>
#include <objects/mmdb3/Residue_interval_pntr.hpp>
#include <objects/mmdb3/Biostruc_feature_id.hpp>
#include <objects/mmdb1/Molecule_id.hpp>
#include <objects/mmdb1/Residue_id.hpp>
#include <objects/mmdb1/Atom_id.hpp>
#include <objects/general/Object_id.hpp>

#include "remove_header_conflicts.hpp"

#include "chemical_graph.hpp"
#include "asn_reader.hpp"
#include "molecule.hpp"
#include "bond.hpp"
#include "structure_set.hpp"
#include "opengl_renderer.hpp"
#include "coord_set.hpp"
#include "atom_set.hpp"
#include "object_3d.hpp"
#include "cn3d_tools.hpp"
#include "molecule_identifier.hpp"

USING_NCBI_SCOPE;
USING_SCOPE(objects);


BEGIN_SCOPE(Cn3D)

static const CBiostruc_residue_graph_set* standardDictionary = NULL;

void LoadStandardDictionary(const char *filename)
{
    standardDictionary = new CBiostruc_residue_graph_set;

    // read dictionary file
    string err;
    if (!ReadASNFromFile(filename,
            const_cast<CBiostruc_residue_graph_set*>(standardDictionary), true, &err))
        FATALMSG("Error reading standard residue dictionary: " << err);

    // make sure it's the right thing
    if (!standardDictionary->IsSetId() ||
        !standardDictionary->GetId().front().GetObject().IsOther_database() ||
        standardDictionary->GetId().front().GetObject().GetOther_database().GetDb() != "Standard residue dictionary" ||
        !standardDictionary->GetId().front().GetObject().GetOther_database().GetTag().IsId() ||
        standardDictionary->GetId().front().GetObject().GetOther_database().GetTag().GetId() != 1)
        FATALMSG("file '" << filename << "' does not contain expected dictionary data");
}

void DeleteStandardDictionary(void)
{
    if (standardDictionary) {
        delete standardDictionary;
        standardDictionary = NULL;
    }
}


ChemicalGraph::ChemicalGraph(StructureBase *parent, const CBiostruc_graph& graph,
    const FeatureList& features) :
    StructureBase(parent), displayListOtherStart(OpenGLRenderer::NO_LIST)
{
    if (!standardDictionary) {
        FATALMSG("need to load standard dictionary first");
        return;
    }

    // figure out what models we'll be drawing, based on contents of parent
    // StructureSet and StructureObject
    const StructureObject *object;
    if (!GetParentOfType(&object)) return;

    // get any name from description
    if (graph.IsSetDescr()) {
        CBiostruc_graph::TDescr::const_iterator d, de = graph.GetDescr().end();
        for (d=graph.GetDescr().begin(); d!=de; ++d) {
            if ((*d)->IsName()) {
                name = (*d)->GetName();
                break;
            }
        }
    }

    // if this is the only StructureObject in this StructureSet, and if this
    // StructureObject has only one CoordSet, and if this CoordSet's AtomSet
    // has multiple ensembles, then use multiple altConf ensemble
    int nAlts = 0;
    if (!parentSet->IsMultiStructure() && object->coordSets.size() == 1 &&
        object->coordSets.front()->atomSet->ensembles.size() > 1) {

        AtomSet *atomSet = object->coordSets.front()->atomSet;
        AtomSet::EnsembleList::iterator e, ee=atomSet->ensembles.end();
        for (e=atomSet->ensembles.begin(); e!=ee; ++e) {
            atomSetList.push_back(make_pair(atomSet, *e));
            ++nAlts;
        }

    // otherwise, use all CoordSets using default altConf ensemble for single
    // structure; for multiple structure StructureSet, use only first CoordSet
    } else {
        StructureObject::CoordSetList::const_iterator c, ce=object->coordSets.end();
        for (c=object->coordSets.begin(); c!=ce; ++c) {
            atomSetList.push_back(make_pair((*c)->atomSet,
                reinterpret_cast<const string *>(NULL)));   // VC++ requires this cast for some reason...
            ++nAlts;
            if (parentSet->IsMultiStructure()) break;
        }
    }
    TRACEMSG("nAlts = " << nAlts);
    if (!nAlts) {
        WARNINGMSG("ChemicalGraph has zero AtomSets!");
        return;
    }

    unsigned int firstNewFrame = parentSet->frameMap.size();
    parentSet->frameMap.resize(firstNewFrame + nAlts);

    // load molecules from SEQUENCE OF Molecule-graph
    CBiostruc_graph::TMolecule_graphs::const_iterator i, ie=graph.GetMolecule_graphs().end();
    for (i=graph.GetMolecule_graphs().begin(); i!=ie; ++i) {
        Molecule *molecule = new Molecule(this,
            i->GetObject(),
            standardDictionary->GetResidue_graphs(),
            graph.GetResidue_graphs());

        if (molecules.find(molecule->id) != molecules.end())
            ERRORMSG("confused by repeated Molecule-graph ID's");
        molecules[molecule->id] = molecule;

        // set molecules' display list; each protein or nucleotide molecule
        // gets its own display list (one display list for each molecule for
        // each set of coordinates), while everything else - hets, solvents,
        // inter-molecule bonds - goes in a single list.
        for (int n=0; n<nAlts; ++n) {

            if (molecule->IsProtein() || molecule->IsNucleotide()) {

                molecule->displayLists.push_back(++(parentSet->lastDisplayList));
                parentSet->frameMap[firstNewFrame + n].push_back(parentSet->lastDisplayList);
                parentSet->transformMap[parentSet->lastDisplayList] = &(object->transformToMaster);

            } else { // het/solvent
                if (displayListOtherStart == OpenGLRenderer::NO_LIST) {
                    displayListOtherStart = parentSet->lastDisplayList + 1;
                    parentSet->lastDisplayList += nAlts;
                }
                molecule->displayLists.push_back(displayListOtherStart + n);
            }
        }
    }

    // load connections from SEQUENCE OF Inter-residue-bond OPTIONAL
    if (graph.IsSetInter_molecule_bonds()) {
        CBiostruc_graph::TInter_molecule_bonds::const_iterator j, je=graph.GetInter_molecule_bonds().end();
        for (j=graph.GetInter_molecule_bonds().begin(); j!=je; ++j) {

            int order = j->GetObject().IsSetBond_order() ?
                j->GetObject().GetBond_order() : Bond::eUnknown;
            const Bond *bond = MakeBond(this,
                j->GetObject().GetAtom_id_1(),
                j->GetObject().GetAtom_id_2(),
                order);
            if (bond) interMoleculeBonds.push_back(bond);

            if (CheckForDisulfide(NULL,
                    j->GetObject().GetAtom_id_1(), j->GetObject().GetAtom_id_2(),
                    &interMoleculeBonds, const_cast<Bond*>(bond), this) ||
                (bond && bond->order == Bond::eRealDisulfide)) {
                // set inter-molecule bonds' display list(s) if real bond or virtual disulfide created
                if (displayListOtherStart == OpenGLRenderer::NO_LIST) {
                    displayListOtherStart = parentSet->lastDisplayList + 1;
                    parentSet->lastDisplayList += nAlts;
                }
            }
        }
    }

    // if hets/solvent/i-m bonds present, add display lists to frames
    if (displayListOtherStart != OpenGLRenderer::NO_LIST) {
        for (int n=0; n<nAlts; ++n) {
            parentSet->frameMap[firstNewFrame + n].push_back(displayListOtherStart + n);
            parentSet->transformMap[displayListOtherStart + n] = &(object->transformToMaster);
        }
    }

    // fill out secondary structure and domain maps from NCBI assigments (in feature block)
    FeatureList::const_iterator l, le = features.end();
    for (l=features.begin(); l!=le; ++l) {
        if (l->GetObject().IsSetDescr()) {

            // find and unpack NCBI sec. struc. or domain features
            CBiostruc_feature_set::TDescr::const_iterator d, de = l->GetObject().GetDescr().end();
            for (d=l->GetObject().GetDescr().begin(); d!=de; ++d) {
                if (d->GetObject().IsName()) {
                    if (d->GetObject().GetName() == "NCBI assigned secondary structure")
                        UnpackSecondaryStructureFeatures(l->GetObject());
                    else if (d->GetObject().GetName() == "NCBI Domains")
                        UnpackDomainFeatures(l->GetObject());
                    break;
                }
            }
        }
    }

    // Assign a (new) domain ID to all residues in a chain, if there are not any domains
    // already set; in other words, assume a chain with no given domain features is composed
    // of a single domain.
    list < int > moleculeIDs;
    MoleculeMap::iterator m, me = molecules.end();
    for (m=molecules.begin(); m!=me; ++m) {
        if (m->second->IsProtein() || m->second->IsNucleotide())
            moleculeIDs.push_back(m->first);
    }
    moleculeIDs.sort();    // assign in order of ID
    list < int >::const_iterator id, ide = moleculeIDs.end();
    for (id=moleculeIDs.begin(); id!=ide; ++id) {
        unsigned int r;
        Molecule *molecule = const_cast<Molecule*>(molecules[*id]);
        for (r=0; r<molecule->residues.size(); ++r)
            if (molecule->residueDomains[r] != Molecule::NO_DOMAIN_SET) break;
        if (r == molecule->residues.size()) {
            int domainID = ++((const_cast<StructureSet*>(parentSet))->nDomains);
            ++(molecule->nDomains);
            (const_cast<StructureObject*>(object))->domainMap[domainID] = molecule;
            (const_cast<StructureObject*>(object))->domainID2MMDB[domainID] = -1;
            for (r=0; r<molecule->residues.size(); ++r)
                molecule->residueDomains[r] = domainID;
        }
    }
}

void ChemicalGraph::UnpackDomainFeatures(const CBiostruc_feature_set& featureSet)
{
    TRACEMSG("unpacking NCBI domain features");
    CBiostruc_feature_set::TFeatures::const_iterator f, fe = featureSet.GetFeatures().end();
    for (f=featureSet.GetFeatures().begin(); f!=fe; ++f) {
        if (f->GetObject().GetType() == CBiostruc_feature::eType_subgraph &&
            f->GetObject().IsSetLocation() &&
            f->GetObject().GetLocation().IsSubgraph() &&
            f->GetObject().GetLocation().GetSubgraph().IsResidues() &&
            f->GetObject().GetLocation().GetSubgraph().GetResidues().IsInterval()) {

            // assign a "domain ID" successively (from 1) over the whole StructureSet
            int domainID = ++((const_cast<StructureSet*>(parentSet))->nDomains);

            // find molecule and set regions, warning about overlaps
            Molecule *molecule = NULL;
            CResidue_pntrs::TInterval::const_iterator i,
                ie = f->GetObject().GetLocation().GetSubgraph().GetResidues().GetInterval().end();
            for (i=f->GetObject().GetLocation().GetSubgraph().GetResidues().GetInterval().begin(); i!=ie; ++i) {
                MoleculeMap::const_iterator m = molecules.find(i->GetObject().GetMolecule_id().Get());
                if (m == molecules.end() ||
                    m->second->id !=    // check to make sure all intervals are on same molecule
                        f->GetObject().GetLocation().GetSubgraph().GetResidues().GetInterval().
                            front().GetObject().GetMolecule_id().Get()) {
                    WARNINGMSG("Bad moleculeID in domain interval");
                    continue;
                }
                molecule = const_cast<Molecule*>(m->second);
                for (int r=i->GetObject().GetFrom().Get()-1; r<=i->GetObject().GetTo().Get()-1; ++r) {
                    if (r < 0 || r >= (int)molecule->residues.size()) {
                        ERRORMSG("Bad residue range in domain feature for moleculeID "
                            << molecule->id << " residueID " << r+1);
                        break;
                    } else if (molecule->residueDomains[r] != Molecule::NO_DOMAIN_SET) {
                        WARNINGMSG("Overlapping domain feature for moleculeID "
                            << molecule->id << " residueID " << r+1);
                        break;
                    } else {
                        molecule->residueDomains[r] = domainID;
                    }
                }
            }

            if (molecule) {
                const StructureObject *object;
                if (!GetParentOfType(&object)) return;
                (const_cast<StructureObject*>(object))->domainMap[domainID] = molecule;
                (const_cast<StructureObject*>(object))->domainID2MMDB[domainID] =
                    f->GetObject().GetId().Get();
                ++(molecule->nDomains);
            }
        }
    }
}

void ChemicalGraph::UnpackSecondaryStructureFeatures(const CBiostruc_feature_set& featureSet)
{
    TRACEMSG("unpacking NCBI sec. struc. features");
    CBiostruc_feature_set::TFeatures::const_iterator f, fe = featureSet.GetFeatures().end();
    for (f=featureSet.GetFeatures().begin(); f!=fe; ++f) {
        if ((f->GetObject().GetType() == CBiostruc_feature::eType_helix ||
                f->GetObject().GetType() == CBiostruc_feature::eType_strand) &&
            f->GetObject().IsSetLocation() &&
            f->GetObject().GetLocation().IsSubgraph() &&
            f->GetObject().GetLocation().GetSubgraph().IsResidues() &&
            f->GetObject().GetLocation().GetSubgraph().GetResidues().IsInterval()) {

            // find molecule and set region, warning about overlaps
            if (f->GetObject().GetLocation().GetSubgraph().GetResidues().GetInterval().size() > 1) {
                WARNINGMSG("Can't deal with multi-interval sec. struc. regions");
                continue;
            }
            const CResidue_interval_pntr& interval =
                f->GetObject().GetLocation().GetSubgraph().GetResidues().GetInterval().front().GetObject();
            MoleculeMap::const_iterator m = molecules.find(interval.GetMolecule_id().Get());
            if (m == molecules.end()) {
                WARNINGMSG("Bad moleculeID in sec. struc. interval");
                continue;
            }
            Molecule *molecule = const_cast<Molecule*>(m->second);
            for (int r=interval.GetFrom().Get()-1; r<=interval.GetTo().Get()-1; ++r) {
                if (r < 0 || r >= (int)molecule->residues.size()) {
                    ERRORMSG("Bad residue range in sec. struc. feature for moleculeID "
                        << molecule->id << " residueID " << r+1);
                    break;
                } if (molecule->residueSecondaryStructures[r] != Molecule::eCoil) {
                    WARNINGMSG("Overlapping sec. struc. feature at moleculeID "
                        << molecule->id << " residueID " << r+1);
                } else {
                    molecule->residueSecondaryStructures[r] =
                        (f->GetObject().GetType() == CBiostruc_feature::eType_helix) ?
                            Molecule::eHelix : Molecule::eStrand;
                }
            }
        }
    }
}

static int moleculeToRedraw = -1;

void ChemicalGraph::RedrawMolecule(int moleculeID) const
{
    moleculeToRedraw = moleculeID;
    DrawAll(NULL);
    moleculeToRedraw = -1;
}

// This is where the work of breaking objects up into display lists gets done.
bool ChemicalGraph::DrawAll(const AtomSet *ignored) const
{
    const StructureObject *object;
    if (!GetParentOfType(&object)) return false;

    if (moleculeToRedraw != -1)
        TRACEMSG("drawing molecule " << moleculeToRedraw
            << " of ChemicalGraph of object " << object->pdbID);
    else
        TRACEMSG("drawing ChemicalGraph of object " << object->pdbID);

    // put each protein (with its 3d-objects) or nucleotide chain in its own display list
    bool continueDraw = false;
    AtomSetList::const_iterator a, ae=atomSetList.end();
    MoleculeMap::const_iterator m, me=molecules.end();
    for (m=molecules.begin(); m!=me; ++m) {
        if (!m->second->IsProtein() && !m->second->IsNucleotide()) continue;
        if (moleculeToRedraw >= 0 && m->second->id != moleculeToRedraw) continue;

        Molecule::DisplayListList::const_iterator md=m->second->displayLists.begin();
        for (a=atomSetList.begin(); a!=ae; ++a, ++md) {

            // start new display list
            //TESTMSG("drawing molecule #" << m->second->id << " + its objects in display list " << *md
            //        << " of " << atomSetList.size());
            parentSet->renderer->StartDisplayList(*md);

            // draw this molecule with all alternative AtomSets (e.g., NMR's or altConfs)
            a->first->SetActiveEnsemble(a->second);
            continueDraw = m->second->DrawAllWithTerminiLabels(a->first);

            if (continueDraw) {
                // find 3D objects for this molecule/CoordSet
                const CoordSet *coordSet;
                if (a->first->GetParentOfType(&coordSet)) {
                    CoordSet::Object3DMap::const_iterator objList = coordSet->objectMap.find(m->second->id);
                    if (objList != coordSet->objectMap.end()) {
                        CoordSet::Object3DList::const_iterator o, oe=objList->second.end();
                        for (o=objList->second.begin(); o!=oe; ++o) {
                            if (!(continueDraw = (*o)->Draw(a->first))) break;
                        }
                    }
                }
            }

            // end display list
            parentSet->renderer->EndDisplayList();

            if (!continueDraw) return false;
        }

        // we're done if this was the single molecule meant to be redrawn
        if (moleculeToRedraw >= 0) break;
    }

    // then put everything else (solvents, hets, and intermolecule bonds) in a single display list
    if (displayListOtherStart == OpenGLRenderer::NO_LIST) return true;
    //TESTMSG("drawing hets/solvents/i-m bonds");

    // always redraw all these even if only a single molecule is to be redrawn -
    // that way connections can show/hide in cases where a particular residue
    // changes its display
    int n = 0;
    for (a=atomSetList.begin(); a!=ae; ++a, ++n) {

        a->first->SetActiveEnsemble(a->second);
        parentSet->renderer->StartDisplayList(displayListOtherStart + n);

        continueDraw = true;
        for (m=molecules.begin(); m!=me; ++m) {
            if (m->second->IsProtein() || m->second->IsNucleotide()) continue;
            if (!(continueDraw = m->second->DrawAll(a->first))) break;
        }

        if (continueDraw) {
            BondList::const_iterator b, be=interMoleculeBonds.end();
            for (b=interMoleculeBonds.begin(); b!=be; ++b) {
                if (!(continueDraw = (*b)->Draw(a->first))) break;
            }
        }

        parentSet->renderer->EndDisplayList();

        if (!continueDraw) return false;
    }

    return true;
}

bool ChemicalGraph::CheckForDisulfide(const Molecule *molecule,
    const CAtom_pntr& atomPtr1, const CAtom_pntr& atomPtr2,
    list < const Bond * > *bondList, Bond *bond, StructureBase *parent)
{
    if (atomPtr1.GetMolecule_id().Get() == atomPtr2.GetMolecule_id().Get() &&
        atomPtr1.GetResidue_id().Get() == atomPtr2.GetResidue_id().Get()) return false;

    const Molecule *mol1, *mol2;
    if (molecule) { // when called from Molecule::Molecule() on interresidue (intramolecular) bond
        mol1 = mol2 = molecule;
    } else {        // when called from ChemicalGraph::ChemicalGraph() on intermolecule bond
        MoleculeMap::const_iterator
            m1 = molecules.find(atomPtr1.GetMolecule_id().Get()),
            m2 = molecules.find(atomPtr2.GetMolecule_id().Get());
        if (m1 == molecules.end() || m2 == molecules.end()) {
            ERRORMSG("ChemicalGraph::CheckForDisulfide() - bad molecule ID");
            return false;
        }
        mol1 = m1->second;
        mol2 = m2->second;
    }

    Molecule::ResidueMap::const_iterator
        res1 = mol1->residues.find(atomPtr1.GetResidue_id().Get()),
        res2 = mol2->residues.find(atomPtr2.GetResidue_id().Get());
    if (res1 == mol1->residues.end() || res2 == mol2->residues.end()) {
        ERRORMSG("ChemicalGraph::CheckForDisulfide() - bad residue ID");
        return false;
    }

    // check to make sure both residues are cysteine
    if (res1->second->type != Residue::eAminoAcid || res1->second->code != 'C' ||
        res2->second->type != Residue::eAminoAcid || res2->second->code != 'C') return false;

    const Residue::AtomInfo
        *atom1 = res1->second->GetAtomInfo(atomPtr1.GetAtom_id().Get()),
        *atom2 = res2->second->GetAtomInfo(atomPtr2.GetAtom_id().Get());
    if (!atom1 || !atom2) {
        ERRORMSG("ChemicalGraph::CheckForDisulfide() - bad atom ID");
        return false;
    }

    // check to make sure both atoms are sulfur, and that residue has an alpha
    if (atom1->atomicNumber != 16 || res1->second->alphaID == Residue::NO_ALPHA_ID ||
        atom2->atomicNumber != 16 || res2->second->alphaID == Residue::NO_ALPHA_ID) return false;

    TRACEMSG("found disulfide between molecule " << atomPtr1.GetMolecule_id().Get()
        << " residue " << atomPtr1.GetResidue_id().Get()
        << " and molecule " << atomPtr2.GetMolecule_id().Get()
        << " residue " << atomPtr2.GetResidue_id().Get());

    // first flag this bond as "real disulfide", so it's not drawn with connection style
    if (bond) bond->order = Bond::eRealDisulfide;

    // then, make a new virtual disulfide bond between the alphas of these residues
    const Bond *virtualDisulfide = MakeBond(parent,
        atomPtr1.GetMolecule_id().Get(), atomPtr1.GetResidue_id().Get(), res1->second->alphaID,
        atomPtr2.GetMolecule_id().Get(), atomPtr2.GetResidue_id().Get(), res2->second->alphaID,
        Bond::eVirtualDisulfide);
    if (!virtualDisulfide) return false;

    bondList->push_back(virtualDisulfide);
    return true;
}

END_SCOPE(Cn3D)
