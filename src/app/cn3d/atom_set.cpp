/*  $Id: atom_set.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
*      Classes to hold sets of atomic data
*
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include <objects/mmdb1/Molecule_id.hpp>
#include <objects/mmdb1/Residue_id.hpp>
#include <objects/mmdb1/Atom_id.hpp>
#include <objects/mmdb2/Model_space_points.hpp>
#include <objects/mmdb2/Atomic_temperature_factors.hpp>
#include <objects/mmdb2/Atomic_occupancies.hpp>
#include <objects/mmdb2/Alternate_conformation_ids.hpp>
#include <objects/mmdb2/Alternate_conformation_id.hpp>
#include <objects/mmdb2/Anisotro_temperatu_factors.hpp>
#include <objects/mmdb2/Isotropi_temperatu_factors.hpp>
#include <objects/mmdb2/Conformation_ensemble.hpp>
#include <objects/mmdb3/Atom_pntrs.hpp>

#include "remove_header_conflicts.hpp"

#include "atom_set.hpp"
#include "vector_math.hpp"
#include "cn3d_tools.hpp"
#include "structure_set.hpp"

USING_NCBI_SCOPE;
USING_SCOPE(objects);


BEGIN_SCOPE(Cn3D)

AtomSet::AtomSet(StructureBase *parent, const CAtomic_coordinates& coords) :
    StructureBase(parent), activeEnsemble(NULL)
{
    unsigned int nAtoms = coords.GetNumber_of_points();
    TRACEMSG("model has " << nAtoms << " atomic coords");

    bool haveTemp = coords.IsSetTemperature_factors(),
         haveOccup = coords.IsSetOccupancies(),
         haveAlt = coords.IsSetAlternate_conf_ids();

    // sanity check
    if (coords.GetAtoms().GetMolecule_ids().size()!=nAtoms ||
        coords.GetAtoms().GetResidue_ids().size()!=nAtoms ||
        coords.GetAtoms().GetAtom_ids().size()!=nAtoms ||
        coords.GetSites().GetX().size()!=nAtoms ||
        coords.GetSites().GetY().size()!=nAtoms ||
        coords.GetSites().GetZ().size()!=nAtoms ||
        (haveTemp &&
            ((coords.GetTemperature_factors().IsIsotropic() &&
              coords.GetTemperature_factors().GetIsotropic().GetB().size()!=nAtoms) ||
             (coords.GetTemperature_factors().IsAnisotropic() &&
              (coords.GetTemperature_factors().GetAnisotropic().GetB_11().size()!=nAtoms ||
               coords.GetTemperature_factors().GetAnisotropic().GetB_12().size()!=nAtoms ||
               coords.GetTemperature_factors().GetAnisotropic().GetB_13().size()!=nAtoms ||
               coords.GetTemperature_factors().GetAnisotropic().GetB_22().size()!=nAtoms ||
               coords.GetTemperature_factors().GetAnisotropic().GetB_23().size()!=nAtoms ||
               coords.GetTemperature_factors().GetAnisotropic().GetB_33().size()!=nAtoms)))) ||
        (haveOccup && coords.GetOccupancies().GetO().size()!=nAtoms) ||
        (haveAlt && coords.GetAlternate_conf_ids().Get().size()!=nAtoms))
        ERRORMSG("AtomSet: confused by list length mismatch");

    // atom-pntr iterators
    CAtom_pntrs::TMolecule_ids::const_iterator
        i_mID = coords.GetAtoms().GetMolecule_ids().begin();
    CAtom_pntrs::TResidue_ids::const_iterator
        i_rID = coords.GetAtoms().GetResidue_ids().begin();
    CAtom_pntrs::TAtom_ids::const_iterator
        i_aID = coords.GetAtoms().GetAtom_ids().begin();

    // atom site iterators
    double siteScale = static_cast<double>(coords.GetSites().GetScale_factor());
    CModel_space_points::TX::const_iterator i_X = coords.GetSites().GetX().begin();
    CModel_space_points::TY::const_iterator i_Y = coords.GetSites().GetY().begin();
    CModel_space_points::TZ::const_iterator i_Z = coords.GetSites().GetZ().begin();

    // occupancy iterator
    CAtomic_occupancies::TO::const_iterator i_occup;
    double occupScale = 0.0;
    if (haveOccup) {
        occupScale = static_cast<double>(coords.GetOccupancies().GetScale_factor());
        i_occup = coords.GetOccupancies().GetO().begin();
    }

    // altConfID iterator
    CAlternate_conformation_ids::Tdata::const_iterator i_alt;
    if (haveAlt) i_alt = coords.GetAlternate_conf_ids().Get().begin();

    // temperature iterators
    double tempScale = 0.0;
    CIsotropic_temperature_factors::TB::const_iterator i_tempI;
    CAnisotropic_temperature_factors::TB_11::const_iterator i_tempA11;
    CAnisotropic_temperature_factors::TB_12::const_iterator i_tempA12;
    CAnisotropic_temperature_factors::TB_13::const_iterator i_tempA13;
    CAnisotropic_temperature_factors::TB_22::const_iterator i_tempA22;
    CAnisotropic_temperature_factors::TB_23::const_iterator i_tempA23;
    CAnisotropic_temperature_factors::TB_33::const_iterator i_tempA33;
    if (haveTemp) {
        if (coords.GetTemperature_factors().IsIsotropic()) {
            tempScale = static_cast<double>
                (coords.GetTemperature_factors().GetIsotropic().GetScale_factor());
            i_tempI = coords.GetTemperature_factors().GetIsotropic().GetB().begin();
        } else {
            tempScale = static_cast<double>
                (coords.GetTemperature_factors().GetAnisotropic().GetScale_factor());
            i_tempA11 = coords.GetTemperature_factors().GetAnisotropic().GetB_11().begin();
            i_tempA12 = coords.GetTemperature_factors().GetAnisotropic().GetB_12().begin();
            i_tempA13 = coords.GetTemperature_factors().GetAnisotropic().GetB_13().begin();
            i_tempA22 = coords.GetTemperature_factors().GetAnisotropic().GetB_22().begin();
            i_tempA23 = coords.GetTemperature_factors().GetAnisotropic().GetB_23().begin();
            i_tempA33 = coords.GetTemperature_factors().GetAnisotropic().GetB_33().begin();
        }
    }

    const StructureObject *constObject;
    if (!GetParentOfType(&constObject)) return;
    StructureObject *object = const_cast<StructureObject*>(constObject);

    // actually do the work of unpacking serial atom data into Atom objects
    for (unsigned int i=0; i<nAtoms; ++i) {
        AtomCoord *atom = new AtomCoord(this);

        atom->site.x = (static_cast<double>(*(i_X++)))/siteScale;
        atom->site.y = (static_cast<double>(*(i_Y++)))/siteScale;
        atom->site.z = (static_cast<double>(*(i_Z++)))/siteScale;
        if (haveOccup)
            atom->occupancy = (static_cast<double>(*(i_occup++)))/occupScale;
        if (haveAlt)
            atom->altConfID = (i_alt++)->Get()[0];
        if (haveTemp) {
            if (coords.GetTemperature_factors().IsIsotropic()) {
                atom->averageTemperature =
                    (static_cast<double>(*(i_tempI++)))/tempScale;
            } else {
                atom->averageTemperature = (
                    (static_cast<double>(*(i_tempA11++))) +
                    (static_cast<double>(*(i_tempA12++))) +
                    (static_cast<double>(*(i_tempA13++))) +
                    (static_cast<double>(*(i_tempA22++))) +
                    (static_cast<double>(*(i_tempA23++))) +
                    (static_cast<double>(*(i_tempA33++)))) / (tempScale * 6.0);
            }
            // track min and max temperatures over whole object
            if (object->minTemperature == StructureObject::NO_TEMPERATURE ||
                atom->averageTemperature < object->minTemperature)
                object->minTemperature = atom->averageTemperature;
            if (object->maxTemperature == StructureObject::NO_TEMPERATURE ||
                atom->averageTemperature > object->maxTemperature)
                object->maxTemperature = atom->averageTemperature;
        }

        // store pointer in map - key+altConfID must be unique
        const AtomPntrKey& key = MakeKey(AtomPntr(
            *(i_mID++),
            *(i_rID++),
            *(i_aID++)));
        if (atomMap.find(key) != atomMap.end()) {
            AtomAltList::const_iterator i_atom, e=atomMap[key].end();
            for (i_atom=atomMap[key].begin(); i_atom!=e; ++i_atom) {
                if ((*i_atom)->altConfID == atom->altConfID)
                    ERRORMSG("confused by multiple atoms of same pntr+altConfID");
            }
        }
        atomMap[key].push_back(atom);

        if (i==0) TRACEMSG("first atom: x " << atom->site.x <<
                ", y " << atom->site.y <<
                ", z " << atom->site.z <<
                ", occup " << atom->occupancy <<
                ", altConfId '" << atom->altConfID << "'" <<
                ", temp " << atom->averageTemperature);
    }

    // get alternate conformer ensembles; store as string of altID characters
    if (haveAlt && coords.IsSetConf_ensembles()) {
        CAtomic_coordinates::TConf_ensembles::const_iterator i_ensemble,
            e_ensemble = coords.GetConf_ensembles().end();
        for (i_ensemble=coords.GetConf_ensembles().begin(); i_ensemble!=e_ensemble; ++i_ensemble) {
            const CConformation_ensemble& ensemble = (*i_ensemble).GetObject();
            string *ensembleStr = new string();
            CConformation_ensemble::TAlt_conf_ids::const_iterator i_altIDs,
                e_altIDs = ensemble.GetAlt_conf_ids().end();
            for (i_altIDs=ensemble.GetAlt_conf_ids().begin();
                 i_altIDs!=e_altIDs; ++i_altIDs) {
                (*ensembleStr) += i_altIDs->Get()[0];
            }
            ensembles.push_back(ensembleStr);
            TRACEMSG("alt conf ensemble '" << (*ensembleStr) << "'");
        }
    }
}

AtomSet::~AtomSet(void)
{
    EnsembleList::iterator i, e=ensembles.end();
    for (i=ensembles.begin(); i!=e; ++i)
        delete const_cast<string *>(*i);
}

const double AtomCoord::NO_TEMPERATURE = -1.0;
const double AtomCoord::NO_OCCUPANCY = -1.0;
const char AtomCoord::NO_ALTCONFID = '-';

bool AtomSet::SetActiveEnsemble(const string *ensemble)
{
    // if not NULL, make sure it's one of this AtomSet's ensembles
    if (ensemble) {
        EnsembleList::const_iterator e, ee=ensembles.end();
        for (e=ensembles.begin(); e!=ee; ++e) {
            if (*e == ensemble) break;
        }
        if (e == ee) {
            ERRORMSG("AtomSet::SetActiveEnsemble received invalid ensemble");
            return false;
        }
    }
    activeEnsemble = ensemble;
    return true;
}

const AtomCoord* AtomSet::GetAtom(const AtomPntr& ap,
    bool getAny, bool suppressWarning) const
{
    AtomMap::const_iterator atomConfs = atomMap.find(MakeKey(ap));
    if (atomConfs == atomMap.end()) {
        if (!suppressWarning)
            WARNINGMSG("can't find atom(s) from pointer (" << ap.mID << ','
                             << ap.rID << ',' << ap.aID << ')');
        return NULL;
    }
    AtomAltList::const_iterator atom = atomConfs->second.begin();

    // if no activeEnsemble specified, just return first altConf
    if (!activeEnsemble) return *atom;

    // otherwise, try to find first atom whose altConfID is in the activeEnsemble
    AtomAltList::const_iterator e = atomConfs->second.end();
    for (; atom!=e; ++atom) {
        if (activeEnsemble->find((*atom)->altConfID) != activeEnsemble->npos)
            return *atom;
    }

    // if none found, but getAny is true, just return the first of the list
    if (getAny) return atomConfs->second.front();

    return NULL;
}

AtomCoord::AtomCoord(StructureBase *parent) :
	StructureBase(parent),
    averageTemperature(NO_TEMPERATURE),
    occupancy(NO_OCCUPANCY),
    altConfID(NO_ALTCONFID)
{
}

END_SCOPE(Cn3D)
