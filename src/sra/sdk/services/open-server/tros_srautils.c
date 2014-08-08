/*===========================================================================
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
*/

#include <tros_srautils.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>

#include <kdb/database.h>
#include <kdb/table.h>
#include <kdb/meta.h>
#include <pthread.h>


static CS_INT		intlen=sizeof(CS_INT);
static CS_INT		tinyintlen=sizeof(CS_TINYINT);
static CS_INT		smallintlen=sizeof(CS_SMALLINT);


static char channel_lbl[][10]={
	"_01(A)",
	"_02(C)",
	"_03(G)",
	"_04(T)",
	"_05",
	"_06",
	"_07",
	"_08",
	"_09",
	"_10",
	"_11",
	"_12",
	"_13",
	"_14",
	"_15",
	"_16",
	"_17",
	"_18",
	"_19",
	"_20"
};
static char channel_lbl_abi[][10]={
        "_01(0)",
        "_02(1)",
        "_03(2)",
        "_04(3)",
        "_05",
        "_06",
        "_07",
        "_08",
        "_09",
        "_10",
        "_11",
        "_12",
        "_13",
        "_14",
        "_15",
        "_16",
        "_17",
        "_18",
        "_19",
        "_20"
};


#define SLX_COORD_LEN 10	

void
SraUserDataDelete(SraUserData *data)
{
	if(!data) return;
	SraUserDataReset(data);
	free(data);
}

static void
SraColumnDataReset(SraColumnData * scd)
{
	if(!scd) return;
	if(scd->sc){SRAColumnRelease(scd->sc);scd->sc=0;}
	scd->not_present=false;
	scd->data=0;
	scd->bitoffset=0;
	scd->bitlen=0;
	scd->len=0;
	scd->ind=CS_NULLDATA;
}

void
SraUserDataReset(SraUserData *data)
{
	if(!data) return;
	SraColumnDataReset(&data->bc);
	SraColumnDataReset(&data->cs);
	SraColumnDataReset(&data->rd);
	SraColumnDataReset(&data->rd_cs);
	SraColumnDataReset(&data->len);
	SraColumnDataReset(&data->name);
    SraColumnDataReset(&data->rdesc);
	SraColumnDataReset(&data->qs1);
	SraColumnDataReset(&data->qs4);
	SraColumnDataReset(&data->pos);
	SraColumnDataReset(&data->sig);
	SraColumnDataReset(&data->rint);
	SraColumnDataReset(&data->nse);
	SraColumnDataReset(&data->cqr);
	SraColumnDataReset(&data->cql);
	SraColumnDataReset(&data->filt);
	SraColumnDataReset(&data->sgrp);
	SraColumnDataReset(&data->trd);
	SraColumnDataReset(&data->lbl);
	SraColumnDataReset(&data->lbls);
	SraColumnDataReset(&data->lbll);
	SraColumnDataReset(&data->delq);
	SraColumnDataReset(&data->delv);
	SraColumnDataReset(&data->subq);
	SraColumnDataReset(&data->subv);
	SraColumnDataReset(&data->insq);
	SraColumnDataReset(&data->frw);
	SraColumnDataReset(&data->frp);
	data->clip_quality_right=0;
	SRATableRelease(data->run);
	data->run=NULL;
	VTableRelease(data->vtbl);
	data->vtbl=NULL;
	VDatabaseRelease(data->vdb);
	data->vdb=NULL;
}

static rc_t
SRATableReadMeta(const SRATable *tbl,char* buf,size_t bsize,size_t *msize,char * metaname)
{
	struct KMDataNode const *node;
	rc_t rc=SRATableOpenMDataNodeRead(tbl,&node,metaname);
	if(rc==0){
		rc=KMDataNodeReadCString(node,buf,bsize,msize);
	}
	return rc;
}

rc_t
SraUserDataSetRunAccession(const SRAMgr *sradb,SraUserData *data,char *acc,char **errstr)
{
	rc_t rc = 0;
	int 	acclen;

    if(!sradb || !data || !acc) {
        return RC(rcRuntime, rcFunction, rcResolving, rcFunctParam, rcNull);
    }
	if(strcmp(data->acc,acc)){
		SraUserDataReset(data);
		strcpy(data->acc,acc);
	}
	acclen=strlen(data->acc);
	if(!data->run){
		size_t size;
		rc=SRAMgrOpenTableRead(sradb,&data->run,acc);
		if(rc!=0) return rc;	
		/*** Fill up metadata - replacement for SRATable ***/
		rc=SRATableMaxSpotId(data->run,&data->run_spot_last);
		if(rc!=0) return rc;
		data->run_spot_first=1;
		{
			const SRAColumn *col;
			bitsz_t offset,len;
			const void	*ptr;

			strcpy(data->platform_str,"UNDEFINED");
                        rc=SRATableOpenColumnRead(data->run,&col,"PLATFORM",vdb_ascii_t);
                        if(rc==0){
                                rc=SRAColumnRead(col,1,&ptr,&offset,&len);
                                if(rc==0){
                                        assert(offset==0);
					memcpy(data->platform_str,ptr,len>>3);
					data->platform_str[len>>3]=0;
				}
                                SRAColumnRelease(col);
                        }
			data->platform = SRA_PLATFORM_UNDEFINED;
			rc=SRATableOpenColumnRead(data->run,&col,"PLATFORM","INSDC:SRA:platform_id");
                        if(rc==0){
                                rc=SRAColumnRead(col,1,&ptr,&offset,&len);
                                if(rc==0){
                                        assert(offset==0);
					assert(len==8);
					data->platform=*(uint8_t*)ptr;
				}
                                SRAColumnRelease(col);
                        }
			data->num_reads = 0;
			rc=SRATableOpenColumnRead(data->run,&col,"READ_LEN","INSDC:coord:len");
                        if(rc==0){
				if(data->platform==SRA_PLATFORM_PACBIO_SMRT) {
					data->num_reads=1;
				} else {
					rc=SRAColumnRead(col,1,&ptr,&offset,&len);
					if(rc==0){
						assert(offset==0);
						assert(len%(sizeof(INSDC_coord_len)*8)==0);
						data->num_reads=len/(sizeof(INSDC_coord_len)*8);
					}
				}
                                SRAColumnRelease(col);
                        }
			if(data->platform==SRA_PLATFORM_PACBIO_SMRT) {
				data->read_mask_bio=1;
			} else {
				data->read_mask_bio=0;
				rc = SraColumnDataOpen(&data->trd,data->run,"READ_TYPE","INSDC:SRA:xread_type",1);
				if(rc==0){
					if(rc==0){
						const uint8_t  *rtype=data->trd.data;
						uint8_t	 i;
						for(i=0; i<data->num_reads;i++){
							if((rtype[i]&SRA_READ_TYPE_BIOLOGICAL)!=0){	
								data->read_mask_bio |= (1<<i);
							}
						}
					}
				}
			}
			data->base_count=0;
			rc=SRATableOpenColumnRead(data->run,&col,"BASE_COUNT",vdb_uint64_t);
                        if(rc==0){
                                rc=SRAColumnRead(col,1,&ptr,&offset,&len);
                                if(rc==0){
                                        assert(offset==0);
                                        assert(len==64);
                                        memcpy(&data->base_count,ptr,len>>3);
                                }
                                SRAColumnRelease(col);
                        }
			data->fixed_spot_len=0;
			rc=SRATableOpenColumnRead(data->run,&col,"FIXED_SPOT_LEN",vdb_uint32_t);
                        if(rc==0){
                                rc=SRAColumnRead(col,1,&ptr,&offset,&len);
                                if(rc==0){
                                        assert(offset==0);
                                        assert(len==32);
                                        memcpy(&data->fixed_spot_len,ptr,len>>3);
                                }
                                SRAColumnRelease(col);
                        }

		}
		
		rc=SRATableReadMeta(data->run,data->key_seq,sizeof(data->key_seq)-1,&size,"MSC454_KEY_SEQUENCE");
		if(rc==0) data->key_seq_len=size; else data->key_seq_len=0;
		/*** start FLOW_CHARS ***/
		size=0;
		rc=SRATableReadMeta(data->run,data->flow_seq,sizeof(data->flow_seq)-1,&size,"MSC454_FLOW_CHARS");
		if(rc!=0){
                        const SRAColumn *fc;
                        void *flow_seq;
                        rc=SRATableOpenColumnRead(data->run,&fc,"FLOW_CHARS",insdc_fasta_t);
                        if(rc==0){
                                bitsz_t offset,len;
                                rc=SRAColumnRead(fc,1,(const void**)&flow_seq,&offset,&len);
                                if(rc==0){
                                        assert(offset==0);
                                        assert(len==data->tdata->num_reads*8);
					size=len>>3;
                                        memcpy(data->flow_seq,flow_seq,size);
				}
                                SRAColumnRelease(fc);
                        }
                }
		data->flow_seq_len=size;
		/*** end FLOW_CHARS ***/

		rc=SRATableReadMeta(data->run,data->linker_seq,sizeof(data->linker_seq)-1,&size,"MSC454_LINKER_SEQUENCE");
		if(rc==0) data->linker_seq_len=size; else data->linker_seq_len=0;

		{
			uint32_t	idx;
			SRANamelist	*typedecls;
			uint32_t 	count;
			data->num_sig_ch=0;
			data->sig_typedecl=NULL;
			rc=SRATableColDatatypes(data->run,"SIGNAL",&idx,&typedecls);
			if(rc==0){
				rc=SRANamelistCount(typedecls,&count);
				if(rc==0){
					if(count==1){
						const char *name;
						rc=SRANamelistGet(typedecls,0,&name);
						if(rc==0){
							if(!strcmp(name,ncbi_fsamp4_t)){
								data->num_sig_ch=4;
								data->sig_typedecl=ncbi_fsamp4_t;
							}else if(!strcmp(name,ncbi_isamp1_t)){
								data->num_sig_ch=1;
								data->sig_typedecl=ncbi_isamp1_t;
							}
						}
					}
				}
				SRANamelistRelease(typedecls);
			}
			data->num_int_ch=0;
			rc=SRATableColDatatypes(data->run,"INTENSITY",&idx,&typedecls);
                        if(rc==0){
                                rc=SRANamelistCount(typedecls,&count);
                                if(rc==0){
                                        if(count==1){
                                                const char *name;
                                                rc=SRANamelistGet(typedecls,0,&name);
                                                if(rc==0){
                                                        if(!strcmp(name,ncbi_fsamp4_t)){
                                                                data->num_int_ch=4;
                                                        }
                                                }
                                        }
                                }
                                SRANamelistRelease(typedecls);
                        }
			data->num_nse_ch=0;
			rc=SRATableColDatatypes(data->run,"NOISE",&idx,&typedecls);
                        if(rc==0){
                                rc=SRANamelistCount(typedecls,&count);
                                if(rc==0){
                                        if(count==1){
                                                const char *name;
                                                rc=SRANamelistGet(typedecls,0,&name);
                                                if(rc==0){
                                                        if(!strcmp(name,ncbi_fsamp4_t)){
                                                                data->num_nse_ch=4;
							
                                                        }
                                                }
                                        }
                                }
                                SRANamelistRelease(typedecls);
                        }
			data->qs4_present=0;
			rc=SRATableColDatatypes(data->run,"QUALITY",&idx,&typedecls);
			if(rc==0){
				rc=SRANamelistCount(typedecls,&count);
                                if(rc==0){	
					int i;
					for(i=0;i<count;i++){
                                                const char *name;
                                                rc=SRANamelistGet(typedecls,i,&name);
                                                if(rc==0){
                                                        if(!strcmp(name,ncbi_qual4_t)){
                                                                data->qs4_present=1;
                                                        }
                                                }
                                        }
                                }
                                SRANamelistRelease(typedecls);
			}
		}
		{
			const SRAColumn *sc;
			void *cs_key;
                	rc=SRATableOpenColumnRead(data->run,&sc,"CS_KEY",insdc_fasta_t);
			if(rc==0){
				bitsz_t offset,len;
				rc=SRAColumnRead(sc,1,(const void**)&cs_key,&offset,&len);
				if(rc==0){
					assert(offset==0);
					assert(len==data->tdata->num_reads*8);
					memcpy(data->cs_key,cs_key,len>>3);
				}
				SRAColumnRelease(sc);
			}
			if(data->platform == SRA_PLATFORM_PACBIO_SMRT){
				data->cqr_present=1;
				data->cql_present=1;
			} else {
				data->cqr_present=0;
				rc=SRATableOpenColumnRead(data->run,&sc,"CLIP_QUALITY_RIGHT","INSDC:coord:one");
				if(rc==0){
					data->cqr_present=1;
					SRAColumnRelease(sc);
				}

				data->cql_present=0;
				rc=SRATableOpenColumnRead(data->run,&sc,"CLIP_QUALITY_LEFT","INSDC:coord:one");
				if(rc==0){
					data->cql_present=1;
					SRAColumnRelease(sc);
				}
			}

			data->spot_group_present=0;
                        rc=SRATableOpenColumnRead(data->run,&sc,"SPOT_GROUP",vdb_ascii_t);
                        if(rc==0){
                                data->spot_group_present=1;
                                SRAColumnRelease(sc);
                        }

			rc=SRATableOpenColumnRead(data->run,&sc,"RD_FILTER",sra_read_filter_t);
			if(rc==0){
				data->rd_filter_present=1;
				SRAColumnRelease(sc);
			} else {
				data->rd_filter_present=0;
			}
		}
		rc=SRATableGetVTableRead (data->run, &data->vtbl );
		if(rc == 0){
			rc=VTableOpenParentRead(data->vtbl,&data->vdb);
			if(rc != 0) data->vdb = NULL;
		} else {
			data->vdb  = NULL; 
			data->vtbl = NULL;
		}
	}
	return 0;
}

rc_t SraColumnDataOpen(SraColumnData *c,const SRATable *t,char* name,char* type,spotid_t id)
{
	rc_t rc = 0;
	if(!c || !t ) {
        return RC(rcRuntime, rcFunction, rcResolving, rcFunctParam, rcNull);
    }
	if(c->sc==NULL && !c->not_present){
		rc=SRATableOpenColumnRead(t,&c->sc,name,type);
		if(GetRCState(rc) == rcNotFound){
			c->not_present=true;
			rc=0;
		} else if(rc != 0){
			return rc;
		}
	}
	if(!c->not_present && (rc=SRAColumnRead(c->sc,id,(const void**)&c->data,&c->bitoffset,&c->bitlen))==0){
		if(c->bitoffset==0){
			c->len=c->bitlen/8;
		} else {
			c->len=0;
		}
		if( c->len == 0){
			c->ind=CS_NULLDATA;
		} else {
			c->ind=CS_GOODDATA;
		}
	} else {
		c->bitoffset=c->bitlen=0;
		c->len=0;
		c->ind=CS_NULLDATA;
	}
	return rc;
}

static void
Sra_utils_parse_coord(unsigned char platform,char *buf,unsigned int spotnamelen,SRASpotCoord* sd)
{
	char buffer[1024];
	int	blen=(spotnamelen < sizeof(buffer)-1)?spotnamelen:sizeof(buffer)-1;
	memcpy(buffer,buf,blen);
	buffer[blen]=0;
        sd->platename_len=0;
        if(platform==SRA_PLATFORM_ILLUMINA || platform==SRA_PLATFORM_ABSOLID) {
                unsigned int a,b,c,d;
                int i;

                for(i=spotnamelen-1;i>=0 && isdigit(buffer[i]);i--);

                if(spotnamelen-i+1 < SLX_COORD_LEN){ /**** New format ***/
                        a=b=c=d=0;
                        for(i=spotnamelen-1;i>=0 && isdigit(buffer[i]);i--);
                        d=atoi(buffer+i+1);
                        if(i>=1){
                        	i--;
                                for(;i>=0 && isdigit(buffer[i]);i--);
                                c=atoi(buffer+i+1);
                                if(i>=1){
					i--;
                                        for(;i>=0 && isdigit(buffer[i]);i--);
                                        b=atoi(buffer+i+1);
                                        if(i>=1 && platform==SRA_PLATFORM_ILLUMINA){
						i--;
                                                for(;i>=0 && isdigit(buffer[i]);i--);
                                                a=atoi(buffer+i+1);
                                        }
                                }
                        }
                        for(;isalpha(buffer[i]);i++); /** get back **/
                        if(i>0){
                                sd->platename_len+=i;
                        }
                } else {
                        sscanf(buffer,"%1x%03x%03x%03x",&a,&b,&c,&d);
                        if(spotnamelen-SLX_COORD_LEN > 0){
                                sd->platename_len+=spotnamelen-SLX_COORD_LEN;
                        }
                }
                sd->lane=a;
                sd->tile=b;
                sd->x=c;
                sd->y=d;
        } else if(platform==SRA_PLATFORM_454){
                int     val=0,i;

                sd->platename_len+=spotnamelen-7;
                sd->tile=(buffer[sd->platename_len]-'0')*10+(buffer[sd->platename_len+1]-'0');
                for(i=sd->platename_len+2;i<spotnamelen;i++){
                        val=36*val;
                        val+=isdigit(buffer[i])?buffer[i]-'0'+26:buffer[i]-'A';
                }
                sd->x=val>>12;
                sd->y=val & 0xfff;
                sd->lane=0;
        } else {
                sd->platename_len=0;
                sd->lane=0;
                sd->tile=0;
                sd->x=0;
                sd->y=0;
        }
}


rc_t SraUserDataOpen(SraUserData *d,spotid_t id,int format_mask,int read_mask)
{
	rc_t rc = 0;
	bool	read_len_needed=false;
	if(!d || id < d->run_spot_first || id > d->run_spot_last)  {
		return RC(rcRuntime, rcFunction, rcResolving, rcFunctParam, rcNull);
	}

	if(format_mask==0) return rc;

	d->spotid=id; /*** save id **/
	if(format_mask & SRA_OUTPUT_NAME_BIT || format_mask & SRA_OUTPUT_COORD_BIT){
		rc=SraColumnDataOpen(&d->name,d->run,"NAME",vdb_ascii_t,id);
		if(rc!=0) return rc;
		if( format_mask & SRA_OUTPUT_NAME_BIT && read_mask != 0 ) {
			rc=SraColumnDataOpen(&d->rdesc,d->run,"READ_DESC",sra_read_desc_t,id);
			if(rc!=0) return rc;
		}
		Sra_utils_parse_coord(d->platform,(char*)d->name.data,d->name.len,&d->coord);
	}
	if(format_mask & SRA_OUTPUT_BASECALL_BIT){
		rc=SraColumnDataOpen(&d->bc,d->run,"READ",insdc_fasta_t,id);
                if(rc!=0) return rc;
		if(read_mask && !d->bc.not_present) read_len_needed=true;
        }
	if(format_mask & SRA_OUTPUT_COLOR_SPACE_BIT){
                rc=SraColumnDataOpen(&d->cs,d->run,"CSREAD",insdc_csfasta_t,id);
                if(rc!=0) return rc;
                if(read_mask && !d->cs.not_present) read_len_needed=true;
        }

	if(format_mask & SRA_OUTPUT_QUALITY4_BIT){
		if(d->platform == SRA_PLATFORM_PACBIO_SMRT){
			rc=SraColumnDataOpen(&d->delq,d->run,"DELETION_QV","U8",id);
			if(rc==0){
				rc=SraColumnDataOpen(&d->delv,d->run,"DELETION_TAG","INSDC:dna:text",id);
				if(rc!=0) return rc;
			}
			rc=SraColumnDataOpen(&d->subq,d->run,"SUBSTITUTION_QV","U8",id);
			if(rc==0){
				rc=SraColumnDataOpen(&d->subv,d->run,"SUBSTITUTION_TAG","INSDC:dna:text",id);
				if(rc!=0) return rc;
			}
			rc=SraColumnDataOpen(&d->insq,d->run,"INSERTION_QV","U8",id);
			rc=SraColumnDataOpen(&d->frw,d->run,"WIDTH_IN_FRAMES","U16",id);
			rc=SraColumnDataOpen(&d->frp,d->run,"PRE_BASE_FRAMES","U16",id);
			rc=0;
		}else{
			rc=SraColumnDataOpen(&d->qs4,d->run,"QUALITY",ncbi_qual4_t,id);
			if(rc!=0) return rc;
			if(read_mask && !d->qs4.not_present) read_len_needed=true;
		}
	}
	if(format_mask & SRA_OUTPUT_QUALITY_BIT){
		rc=SraColumnDataOpen(&d->qs1,d->run,"QUALITY",ncbi_qual1_t,id);
                if(rc!=0) return rc;
		if(read_mask && !d->qs1.not_present) read_len_needed=true;
        }
	if(format_mask & SRA_OUTPUT_POSITION_BIT){
	       rc=SraColumnDataOpen(&d->pos,d->run,"POSITION","NCBI:SRA:pos16",id);
	       if(rc!=0) return rc;
	       if(read_mask && !d->pos.not_present) read_len_needed=true;
	}
	if(format_mask & SRA_OUTPUT_SIGNAL_BIT){
		rc=SraColumnDataOpen(&d->sig,d->run,"SIGNAL",d->sig_typedecl,id);
                if(rc!=0) return rc;
        }
	if(format_mask & SRA_OUTPUT_INTENSITY_BIT){
		rc=SraColumnDataOpen(&d->rint,d->run,"INTENSITY",ncbi_fsamp4_t,id);
		if(rc!=0) return rc;
        }
	if(format_mask & SRA_OUTPUT_NOISE_BIT){
		rc=SraColumnDataOpen(&d->nse,d->run,"NOISE",ncbi_fsamp4_t,id);
                if(rc!=0) return rc;
        }
	if(format_mask & SRA_OUTPUT_READ_TYPE_BIT){
                rc=SraColumnDataOpen(&d->trd,d->run,"READ_TYPE","INSDC:SRA:xread_type",id);
                if(rc!=0) return rc;
        }
	if(format_mask & SRA_OUTPUT_READ_LEN_BIT){
		read_len_needed=true;
        }


	if(format_mask & SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT){
		rc=SraColumnDataOpen(&d->cql,d->run,"TRIM_START","INSDC:coord:one",id);
		if( rc == 0 && d->cql.ind==CS_GOODDATA) {
			rc=SraColumnDataOpen(&d->cqr,d->run,"TRIM_LEN","INSDC:coord:len",id);
			if(rc==0 && d->cqr.ind==CS_GOODDATA){
				if( *(INSDC_coord_len*)d->cqr.data == 0){
					d->clip_quality_left=0;
					d->clip_quality_right=0;
				} else {
					d->clip_quality_left=*(INSDC_coord_one*)d->cql.data;
					d->clip_quality_right = d->clip_quality_left + *(INSDC_coord_len*)d->cqr.data - 1;
				}
				if(d->clip_quality_right < d->clip_quality_left)
					d->clip_quality_right=d->clip_quality_left;
			} else return rc;
		} else return rc;
        }
        if(format_mask & SRA_OUTPUT_FILTER_BIT){
                rc=SraColumnDataOpen(&d->filt,d->run,"READ_FILTER",sra_read_filter_t,id);
                if(rc!=0) return rc;
        }
        if(format_mask & SRA_OUTPUT_SPOTGROUP_BIT){
                rc=SraColumnDataOpen(&d->sgrp,d->run,"SPOT_GROUP",vdb_ascii_t,id);
		rc = 0;
        }
	if(format_mask & SRA_OUTPUT_READ_LABEL_BIT){
                rc=SraColumnDataOpen(&d->lbl,d->run,"LABEL",vdb_ascii_t,id);
                rc=SraColumnDataOpen(&d->lbls,d->run,"LABEL_START","INSDC:coord:zero",id);
                rc=SraColumnDataOpen(&d->lbll,d->run,"LABEL_LEN","INSDC:coord:len",id);	
		rc=0;
        }

	rc=SraColumnDataOpen(&d->len,d->run,"READ_LEN",vdb_uint32_t,id);
	if(rc!=0) return rc;
	return rc;
}

rc_t SraGetFileList(const char* server, const char* rpath, int parent, SraFiles* files, int* max_qty, bool signal, bool names)
{
    rc_t rc = 0;
    DIR* D = NULL;
    struct dirent* d = NULL;
    int x;
    char p[2048];

	if(!server || !rpath || !files )  {
        return RC(rcRuntime, rcFunction, rcResolving, rcFunctParam, rcNull);
    }

    /*fprintf(stderr, "Enter: '%s' '%s' %i %i\n", server, rpath, parent, *max_qty);*/

    snprintf(p, sizeof(p), "%s/%s", server, rpath);
    D = opendir(p);
    if( D == NULL ) {
        rc = RC(rcRuntime, rcDirectory, rcAccessing, rcPath, rcNotFound);
    }
    while( rc == 0 && (d = readdir(D)) != NULL ) {
        struct stat st;

        if( strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 ) {
            continue;
        }
        if( *max_qty - 1 < 0 ) {
            rc = RC(rcRuntime, rcFunction, rcConstructing, rcDirEntry, rcExhausted);
            continue;
        }
        x = snprintf(p, sizeof(p), "%s/%s/%s", server, rpath, d->d_name);
        if( x < 0 ) {
            rc = RC(rcRuntime, rcFunction, rcConstructing, rcPath, rcUnexpected);
            continue;
        } else if( x >= sizeof(p) ) {
            rc = RC(rcRuntime, rcFunction, rcConstructing, rcPath, rcTooLong);
            continue;
        }
        if( stat(p, &st) != 0 ) {
            rc = RC(rcRuntime, rcFile, rcAccessing, rcPath, rcNotFound);
            continue;
        }
        if( d->d_type == DT_DIR ) {
            if( parent > 0 && strcmp(files[parent].name, "col") == 0 ) {
                if( !signal && ( strcmp(d->d_name, "SIGNAL") == 0 ||
                                 strcmp(d->d_name, "POSITION") == 0 ||
                                 strcmp(d->d_name, "NOISE") == 0 ||
                                 strcmp(d->d_name, "INTENSITY") == 0 ) ) {
                    continue;
                }
                /* current SDK version cannot work w/o this cols anyway */
                if( false && !names && ( strcmp(d->d_name, "X") == 0 ||
                                strcmp(d->d_name, "Y") == 0 ) ) {
                    continue;
                }
            }
            *max_qty = *max_qty - 1;
            files[*max_qty].id = *max_qty;
            files[*max_qty].parent = parent;
            files[*max_qty].type = kptDir;
            strncpy(files[*max_qty].name, d->d_name, sizeof(files[*max_qty].name) - 1);
            files[*max_qty].mtime = st.st_mtime;
            files[*max_qty].size = 0;
            x = snprintf(p, sizeof(p), "%s/%s", rpath, d->d_name);
            if( x < 0 ) {
                rc = RC(rcRuntime, rcFunction, rcConstructing, rcPath, rcUnexpected);
                continue;
            } else if( x >= sizeof(p) ) {
                rc = RC(rcRuntime, rcFunction, rcConstructing, rcPath, rcTooLong);
                continue;
            }
            rc = SraGetFileList(server, p, *max_qty, files, max_qty, signal, names);
        } else if( d->d_type == DT_FIFO || d->d_type == DT_CHR || d->d_type == DT_REG || d->d_type == DT_LNK ) {
            *max_qty = *max_qty - 1;
            files[*max_qty].id = *max_qty;
            files[*max_qty].parent = parent;
            files[*max_qty].type = kptFile;
            strncpy(files[*max_qty].name, d->d_name, sizeof(files[*max_qty].name) - 1);
            files[*max_qty].mtime = st.st_mtime;
            files[*max_qty].size = st.st_size;
        }
    }
    if( D != NULL ) {
        closedir(D);
    }
    return rc;
}

/******************* OUTPUT FUNCTIONS *******/
CS_RETCODE
Sra_send_asnprop(SRV_PROC *spp)
{
	CS_DATAFMT      cs_stat_fmt={"state",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_conf_fmt={"confidential",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_supp_fmt={"suppress",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_with_fmt={"withdraw",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_len_fmt={"length",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_own_fmt={"owner",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_user_fmt={"username",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_div_fmt={"div_in",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,3,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_class_fmt={"class",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_hdate_fmt={"hup_date",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_NULLTERM,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_ltm_fmt={"last_touched_m",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};

	CS_TINYINT	state=100,tiny_zero=0;
	CS_SMALLINT	small_zero=0;
	CS_INT		len,int_zero=0;
	CS_CHAR		username[]="unknown";
	CS_CHAR		div_in[]="N/A";
	CS_CHAR		hup_date[]="01/01/1900";
	UserThreadDataPtr tdatap=NULL;
	SraUserData     *sd=NULL;



	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                return CS_FAIL;
        }
        sd=tdatap->userdata;
	if(sd==NULL) return CS_FAIL;

	len=sd->run_spot_last;
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_stat_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_stat_fmt,(CS_BYTE*)&state,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_conf_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_conf_fmt,(CS_BYTE*)&tiny_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_supp_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_supp_fmt,(CS_BYTE*)&tiny_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_with_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_with_fmt,(CS_BYTE*)&small_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_len_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_len_fmt,(CS_BYTE*)&len,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,6,&cs_own_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,6,&cs_own_fmt,(CS_BYTE*)&int_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,7,&cs_user_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,7,&cs_user_fmt,(CS_BYTE*)username,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,8,&cs_div_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,8,&cs_div_fmt,(CS_BYTE*)div_in,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,9,&cs_class_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,9,&cs_class_fmt,(CS_BYTE*)&tiny_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,10,&cs_hdate_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,10,&cs_hdate_fmt,(CS_BYTE*)hup_date,NULL,NULL)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,11,&cs_ltm_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,11,&cs_ltm_fmt,(CS_BYTE*)&int_zero,NULL,NULL)!=CS_SUCCEED
	   || srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED
	   || srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1) != CS_SUCCEED){
		return CS_FAIL;
	}
	return CS_SUCCEED;
}

CS_RETCODE
SraUserSendMetadata(SRV_PROC *spp)
{
	CS_DATAFMT      cs_acc_fmt={"run_acc",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,10,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_platform_fmt={"platform",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,20,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_key_fmt={"key_sequence",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,10,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_flow_fmt={"flow_sequence",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,10000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_flowcnt_fmt={"flow_count",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_nreads_fmt={"num_reads",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_first_fmt={"spotid_first",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_last_fmt={"spotid_last",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_qsch_fmt={"qs_channels",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_qs4ch_fmt={"qs4_channels",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_sigch_fmt={"sig_channels",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_nsech_fmt={"nse_channels",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_intch_fmt={"int_channels",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_cqrp_fmt={"clip_info",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_cqlp_fmt={"clip_info_left",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_linker_fmt={"linker_sequence",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_cskey_fmt={"cs_key",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,32,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_mbases_fmt={"Mbases",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_bases_fmt={"bases",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,50,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_biomask_fmt={"read_mask_bio",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_fixed_len_fmt={"spot_fixed_length",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
    CS_DATAFMT      cs_readfixed_len={"read_fixed_lengths",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_rdfilt_fmt={"read_filter_present",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_sgroup_fmt={"spot_group_present",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_INT		acclen,platformlen,mbases,readlen_len;
	CS_SMALLINT	goodind=0,keyind=0,flowind=0;
	UserThreadDataPtr tdatap=NULL;
	SraUserData     *sd=NULL;
	CS_INT		num_qs_ch=1;
	CS_INT		num_qs4_ch;
	CS_CHAR		bases_str[51];
	CS_INT		bases_str_len;
	CS_INT		flow_count;
    CS_CHAR     readlen_str[32 * 10];
    int i;

	if(srv_thread_props(spp,CS_GET,SRV_T_USERDATA,(CS_VOID PNTR PNTR)&tdatap,sizeof(tdatap),NULL) == CS_FAIL){
                return CS_FAIL;
        }
        sd=tdatap->userdata;
	if(sd==NULL) return CS_FAIL;

	

	acclen=strlen(sd->acc);
	platformlen=strlen(sd->platform_str);
	if(sd->key_seq_len==0) keyind=-1;
	if(sd->flow_seq_len==0) flowind=-1;
	mbases=sd->base_count/1000000;
	sprintf(bases_str,"%ld",sd->base_count);
	bases_str_len=strlen(bases_str);
	num_qs4_ch=sd->qs4_present?1:0;
	flow_count=sd->flow_seq_len>0?sd->flow_seq_len:sd->fixed_spot_len;
    
    readlen_str[0] = '\0';
    for(i = 0; i < sd->num_reads; i++) {
	/*** TODO - redo with column width analyses ****/
        /*** snprintf(readlen_str + strlen(readlen_str), sizeof(readlen_str), "%u,", sd->tdata->read_descr[i].fixed_len);*/
        snprintf(readlen_str + strlen(readlen_str), sizeof(readlen_str), "%u,", 0);
    }
    readlen_str[strlen(readlen_str) - 1] = '\0';
    readlen_len = strlen(readlen_str);
	
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,1,&cs_acc_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,1,&cs_acc_fmt,(CS_BYTE*)sd->acc,&acclen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,2,&cs_platform_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,2,&cs_platform_fmt,(CS_BYTE*)sd->platform_str,&platformlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,3,&cs_first_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,3,&cs_first_fmt,(CS_BYTE*)&sd->run_spot_first,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,4,&cs_last_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,4,&cs_last_fmt,(CS_BYTE*)&sd->run_spot_last,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,5,&cs_nreads_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,5,&cs_nreads_fmt,(CS_BYTE*)&sd->num_reads,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,6,&cs_qsch_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,6,&cs_qsch_fmt,(CS_BYTE*)&num_qs_ch,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,7,&cs_sigch_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,7,&cs_sigch_fmt,(CS_BYTE*)&sd->num_sig_ch,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,8,&cs_key_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,8,&cs_key_fmt,(CS_BYTE*)sd->key_seq,&sd->key_seq_len,&keyind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,9,&cs_flow_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,9,&cs_flow_fmt,(CS_BYTE*)sd->flow_seq,&sd->flow_seq_len,&keyind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,10,&cs_flowcnt_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,10,&cs_flowcnt_fmt,(CS_BYTE*)&flow_count,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,11,&cs_cqrp_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,11,&cs_cqrp_fmt,(CS_BYTE*)&sd->cqr_present,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,12,&cs_linker_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,12,&cs_linker_fmt,(CS_BYTE*)sd->linker_seq,&sd->linker_seq_len,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,13,&cs_mbases_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,13,&cs_mbases_fmt,(CS_BYTE*)&mbases,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,14,&cs_nsech_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,14,&cs_nsech_fmt,(CS_BYTE*)&sd->num_nse_ch,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,15,&cs_intch_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,15,&cs_intch_fmt,(CS_BYTE*)&sd->num_int_ch,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,16,&cs_qs4ch_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,16,&cs_qs4ch_fmt,(CS_BYTE*)&num_qs4_ch,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,17,&cs_bases_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,17,&cs_bases_fmt,(CS_BYTE*)bases_str,&bases_str_len,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,18,&cs_biomask_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,18,&cs_biomask_fmt,(CS_BYTE*)&sd->read_mask_bio,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,19,&cs_cskey_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,19,&cs_cskey_fmt,(CS_BYTE*)sd->cs_key,(CS_INT*)&sd->num_reads,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,20,&cs_fixed_len_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,20,&cs_fixed_len_fmt,(CS_BYTE*)&sd->fixed_spot_len,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,21,&cs_readfixed_len)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,21,&cs_readfixed_len,(CS_BYTE*)readlen_str,&readlen_len,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,22,&cs_rdfilt_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,22,&cs_rdfilt_fmt,(CS_BYTE*)&sd->rd_filter_present,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,23,&cs_sgroup_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,23,&cs_sgroup_fmt,(CS_BYTE*)&sd->spot_group_present,&intlen,&goodind)!=CS_SUCCEED
	   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,24,&cs_cqlp_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,24,&cs_cqlp_fmt,(CS_BYTE*)&sd->cql_present,&intlen,&goodind)!=CS_SUCCEED
	   || srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED 
	   || srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,1) != CS_SUCCEED) {
		return CS_FAIL;
	}
	return CS_SUCCEED;
}
CS_RETCODE
SraUserSpotDataPrepare(SRV_PROC *spp,SraUserData *sd,int format_mask,int read_mask)
{
	CS_DATAFMT      cs_spotid_fmt={"id",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_spotname_fmt={"name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,150,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_spotgroup_fmt={"group",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,150,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_plate_fmt={"plate",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,100,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_sig_fmt={"signal",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,20000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_int_fmt={"intensity",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,20000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_nse_fmt={"noise",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,20000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lane_fmt={"lane",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_tile_fmt={"tile",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_region_fmt={"region",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_x_fmt={"coord_x",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_y_fmt={"coord_y",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_cqr_fmt={"clip_qual_r",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_cql_fmt={"clip_qual_l",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_readtype_fmt={"read_type",CS_NULLTERM,CS_BINARY_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_readlen_fmt={"read_len",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbl_fmt={"label",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbll_fmt={"label_len",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbls_fmt={"label_start",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	int		outcol=0;
	int		i;

	outcol++;
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,outcol,&cs_spotid_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_spotid_fmt,(CS_BYTE*)&sd->spotid,&intlen,NULL)!=CS_SUCCEED) return CS_FAIL;
	if(format_mask & SRA_OUTPUT_NAME_BIT){
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_spotname_fmt)!=CS_SUCCEED)return CS_FAIL;
		sd->name.outcol[0]=outcol;
	}
	if(format_mask & SRA_OUTPUT_COORD_BIT){
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_plate_fmt)!=CS_SUCCEED) return CS_FAIL;
		sd->plate_outcol=outcol;
		if(sd->platform==SRA_PLATFORM_ILLUMINA || sd->platform==SRA_PLATFORM_ABSOLID) {
			if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_lane_fmt)!=CS_SUCCEED
			   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_lane_fmt,(CS_BYTE*)&sd->coord.lane,&tinyintlen,NULL)!=CS_SUCCEED
			   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_tile_fmt)!=CS_SUCCEED
			   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_tile_fmt,(CS_BYTE*)&sd->coord.tile,&smallintlen,NULL)!=CS_SUCCEED
			  ) return CS_FAIL;
		} else if(sd->platform==SRA_PLATFORM_454){
			if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_region_fmt)!=CS_SUCCEED
                           || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_region_fmt,(CS_BYTE*)&sd->coord.tile,&smallintlen,NULL)!=CS_SUCCEED
                          ) return CS_FAIL;
		}
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_x_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_x_fmt,(CS_BYTE*)&sd->coord.x,&smallintlen,NULL)!=CS_SUCCEED
		   || srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_y_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_y_fmt,(CS_BYTE*)&sd->coord.y,&smallintlen,NULL)!=CS_SUCCEED
		) return CS_FAIL;
	}
	if(format_mask & SRA_OUTPUT_CLIP_QUALITY_RIGHT_BIT){
		if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_cqr_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_cqr_fmt,(CS_BYTE*)&sd->clip_quality_right,&smallintlen,NULL)!=CS_SUCCEED) return CS_FAIL;
		if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_cql_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_cql_fmt,(CS_BYTE*)&sd->clip_quality_left,&smallintlen,NULL)!=CS_SUCCEED) return CS_FAIL;
	}
	if(read_mask==0){/*** Special Case - do not split by read ****/
		CS_DATAFMT cs_col_fmt={"",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,10000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
		if(format_mask & SRA_OUTPUT_BASECALL_BIT){
			strcpy(cs_col_fmt.name,"read");
			cs_col_fmt.maxlength=sd->fixed_spot_len?sd->fixed_spot_len:10000;
			cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
			if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
			sd->bc.outcol[0]=outcol;
		}
		if(format_mask & SRA_OUTPUT_COLOR_SPACE_BIT){
                        strcpy(cs_col_fmt.name,"csread");
                        cs_col_fmt.maxlength=sd->fixed_spot_len?sd->fixed_spot_len:10000;
                        cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
                        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
                        sd->cs.outcol[0]=outcol;
                }

		if(format_mask & SRA_OUTPUT_QUALITY_BIT){
			strcpy(cs_col_fmt.name,"qual");
			cs_col_fmt.maxlength=(sd->fixed_spot_len?sd->fixed_spot_len:10000);
			cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
			if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
			sd->qs1.outcol[0]=outcol;
		
		}
		if(format_mask & SRA_OUTPUT_QUALITY4_BIT){
			strcpy(cs_col_fmt.name,"qual4");
			cs_col_fmt.maxlength=4*(sd->fixed_spot_len?sd->fixed_spot_len:10000);
			cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
			if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
			sd->qs4.outcol[0]=outcol;
		}
		if(format_mask & SRA_OUTPUT_POSITION_BIT){
			strcpy(cs_col_fmt.name,"pos");
			cs_col_fmt.maxlength=2*(sd->fixed_spot_len?sd->fixed_spot_len:10000);
			cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
			if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
			sd->pos.outcol[0]=outcol;
		}
                if(format_mask & SRA_OUTPUT_FILTER_BIT){
                        strcpy(cs_col_fmt.name,"filt");
                        cs_col_fmt.maxlength=sd->num_reads;
                        cs_col_fmt.datatype=CS_BINARY_TYPE;
                        if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
                        sd->filt.outcol[0]=outcol;
                }
	} else for(i=0;i<sd->num_reads;i++){
		CS_DATAFMT cs_col_fmt={"",CS_NULLTERM,CS_TEXT_TYPE,CS_FMT_UNUSED,10000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
		if(read_mask & (1<<i)){
            if(format_mask & SRA_OUTPUT_NAME_BIT){
                sprintf(cs_col_fmt.name,"label_%.2d",i);
				cs_col_fmt.maxlength=150;
				cs_col_fmt.datatype=CS_CHAR_TYPE;
                if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->rdesc.outcol[i]=outcol;
            }
			if(format_mask & SRA_OUTPUT_BASECALL_BIT){
				sprintf(cs_col_fmt.name,"read_%.2d",i);
				cs_col_fmt.maxlength=10000;
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
				if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
				sd->bc.outcol[i]=outcol;
			}
			if(format_mask & SRA_OUTPUT_COLOR_SPACE_BIT){
                sprintf(cs_col_fmt.name,"csread_%.2d",i);
                cs_col_fmt.maxlength=10000;
                cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
                if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->cs.outcol[i]=outcol;
            }
			if(format_mask & SRA_OUTPUT_QUALITY_BIT){
				sprintf(cs_col_fmt.name,"qual_%.2d",i);
				cs_col_fmt.maxlength=10000;
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
				sd->qs1.outcol[i]=outcol;
			
			}
			if(format_mask & SRA_OUTPUT_QUALITY4_BIT){
                                sprintf(cs_col_fmt.name,"qual4_%.2d",i);
                                cs_col_fmt.maxlength=4*10000;
                                cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
				sd->qs4.outcol[i]=outcol;
                        }
			if(format_mask & SRA_OUTPUT_POSITION_BIT){
                                sprintf(cs_col_fmt.name,"pos_%.2d",i);
                                cs_col_fmt.maxlength=2*10000;
                                cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
				sd->pos.outcol[i]=outcol;
                        }
                        if(format_mask & SRA_OUTPUT_FILTER_BIT){
                                sprintf(cs_col_fmt.name,"filt_%.2d",i);
                                cs_col_fmt.maxlength=1;
                                cs_col_fmt.datatype=CS_BINARY_TYPE;
                                if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_col_fmt)!=CS_SUCCEED) return CS_FAIL;
                                sd->filt.outcol[i]=outcol;
                        }
		}
	}
	if(format_mask & SRA_OUTPUT_SIGNAL_BIT){
		if(srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_sig_fmt)!=CS_SUCCEED) return CS_FAIL;
		sd->sig.outcol[0]=outcol;
	}
	if(format_mask & SRA_OUTPUT_INTENSITY_BIT){
		if(srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_int_fmt)!=CS_SUCCEED) return CS_FAIL;
		sd->rint.outcol[0]=outcol;
	}
	if(format_mask & SRA_OUTPUT_NOISE_BIT){
		if(srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_nse_fmt)!=CS_SUCCEED) return CS_FAIL;
		sd->nse.outcol[0]=outcol;
	}
	if(format_mask & SRA_OUTPUT_SPOTGROUP_BIT){
		if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_spotgroup_fmt)!=CS_SUCCEED) return CS_FAIL;
		sd->sgrp.outcol[0]=outcol;
	}
	if(format_mask & SRA_OUTPUT_READ_TYPE_BIT){
                if(srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_readtype_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->trd.outcol[0]=outcol;
        }
	if(format_mask & SRA_OUTPUT_READ_LEN_BIT){
                if(srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_readlen_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->len.outcol[0]=outcol;
        }
	if(format_mask & SRA_OUTPUT_READ_LABEL_BIT){
                if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_lbl_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->lbl.outcol[0]=outcol;
		if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_lbls_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->lbls.outcol[0]=outcol;
		if( srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_lbll_fmt)!=CS_SUCCEED) return CS_FAIL;
                sd->lbll.outcol[0]=outcol;
        }
	return CS_SUCCEED;
}
CS_RETCODE
SraUserSpotDataSend(SRV_PROC *spp,SraUserData *sd,int format_mask,int read_mask,int do_clip)
{
	CS_DATAFMT      cs_col_fmt={"",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,sizeof(CS_INT),CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_sig_fmt={"signal",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,200000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_int_fmt={"intensity",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,200000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_nse_fmt={"noise",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,200000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_spotgroup_fmt={"group",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,150,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_spotname_fmt={"name",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,150,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_plate_fmt={"plate",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,100,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_readtype_fmt={"read_type",CS_NULLTERM,CS_BINARY_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_readlen_fmt={"read_len",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbl_fmt={"label",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,255,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbll_fmt={"label_len",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_lbls_fmt={"label_start",CS_NULLTERM,CS_IMAGE_TYPE,CS_FMT_UNUSED,1000,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_INT		numbases;
	int		i;
	CS_INT		lb_len[255],rlen[255],rlen2[255],rlen4[255],filt_len;
	CS_SMALLINT lb_ind[255],bc_ind[255],qs1_ind[255],qs4_ind[255],pos_ind[255],cs_ind[255];
	CS_RETCODE	rc=CS_SUCCEED;
	int		offset;
	int 		used_bases;
	CS_INT		*read_len=(CS_INT*)sd->len.data;
	CS_INT		read_len_len = sd->len.len;
	CS_SMALLINT	read_len_ind=sd->len.ind;
	CS_BOOL		read_len_alloc=false;

	if(   format_mask & SRA_OUTPUT_BASECALL_BIT 
	   || format_mask & SRA_OUTPUT_COLOR_SPACE_BIT 
	   || format_mask & SRA_OUTPUT_QUALITY_BIT 
	   || format_mask & SRA_OUTPUT_QUALITY4_BIT 
	   || format_mask & SRA_OUTPUT_NAME_BIT 
	   || format_mask & SRA_OUTPUT_POSITION_BIT
           || format_mask & SRA_OUTPUT_FILTER_BIT
           || format_mask & SRA_OUTPUT_READ_LEN_BIT
		){

		if(sd->fixed_spot_len>0){
			numbases=sd->fixed_spot_len;
		} else if(sd->bc.len > 0){
			numbases=sd->bc.len;
		} else if(sd->qs1.len > 0){
			numbases=sd->qs1.len;
		} else if(sd->qs4.len > 0){
			numbases=sd->qs4.len/4;
		} else if(sd->pos.len > 0){
			numbases=sd->pos.len/2;
		} else if(sd->cs.len > 0){
			numbases=sd->cs.len;
		} else if(sd->num_reads > 0){
			for(i=0,numbases=0;i < sd->num_reads; i++){
				numbases+= read_len[i];
			}
		} else {
                      	numbases=0;
		}
		if(format_mask & SRA_OUTPUT_NAME_BIT){
			if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->name.outcol[0],&cs_spotname_fmt,
				(CS_BYTE*)sd->name.data,&sd->name.len,&sd->name.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
		}
		read_len_ind =  sd->len.ind;
		read_len_len = sd->len.len;
		if(do_clip) {
			read_len = malloc(read_len_len);
			memcpy(read_len,sd->len.data,read_len_len);
			read_len_alloc=true;
			if(sd->clip_quality_right <= sd->clip_quality_left || sd->clip_quality_left > numbases){
				numbases=0;
				offset=0;
			} else {
				offset=sd->clip_quality_left-1;
				if(sd->clip_quality_right >= numbases) numbases -= offset;
                                else numbases=sd->clip_quality_right-offset;
			}
			for(i=0,used_bases=0;i<sd->num_reads;i++){
				int start = used_bases;
				int stop  = used_bases + read_len[i];
				int len;
				if(start < offset) start=offset;
				if(stop >= offset + numbases) stop = offset + numbases;
				len  = stop - start;
				if(len < 0) len =0;
				read_len[i]=len;
				used_bases=stop;
			}
		} else {
			offset=0;
		}
		if(read_mask==0){
			rlen[0]=numbases;
			rlen2[0]=rlen[0]*2;
			rlen4[0]=rlen[0]*4;
			if(format_mask & SRA_OUTPUT_BASECALL_BIT){
				cs_col_fmt.maxlength=sd->fixed_spot_len?sd->fixed_spot_len:10000;
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
				if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->bc.outcol[0],&cs_col_fmt,
					((CS_BYTE*)sd->bc.data)+offset,rlen,&sd->bc.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
			}
			if(format_mask & SRA_OUTPUT_COLOR_SPACE_BIT){
                                cs_col_fmt.maxlength=sd->fixed_spot_len?sd->fixed_spot_len:10000;
                                cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
                                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->cs.outcol[0],&cs_col_fmt,
                                        ((CS_BYTE*)sd->cs.data)+offset,rlen,&sd->cs.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                        }
			if(format_mask & SRA_OUTPUT_QUALITY_BIT){
				cs_col_fmt.maxlength=sd->fixed_spot_len?sd->fixed_spot_len:10000;
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->qs1.outcol[0],&cs_col_fmt,
					((CS_BYTE*)sd->qs1.data)+offset,rlen,&sd->qs1.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
			}
			if(format_mask & SRA_OUTPUT_QUALITY4_BIT){
				cs_col_fmt.maxlength=4*(sd->fixed_spot_len?sd->fixed_spot_len:10000);
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				rlen4[0]=rlen[0]*4;
				if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->qs4.outcol[0],&cs_col_fmt,
					((CS_BYTE*)sd->qs4.data)+4*offset,rlen4,&sd->qs4.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
			}
			if(format_mask & SRA_OUTPUT_POSITION_BIT){
				cs_col_fmt.maxlength=2*(sd->fixed_spot_len?sd->fixed_spot_len:10000);
				cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
				rlen2[0]=rlen[0]*2;
				if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->pos.outcol[0],&cs_col_fmt,
					((CS_BYTE*)sd->pos.data)+2*offset,rlen2,&sd->pos.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                        }
			if(format_mask & SRA_OUTPUT_FILTER_BIT){
                                cs_col_fmt.maxlength=sd->num_reads;
				cs_col_fmt.datatype=CS_BINARY_TYPE;
				filt_len=sd->num_reads;
				if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->filt.outcol[0],&cs_col_fmt,
					(CS_BYTE*)sd->filt.data,&filt_len,&sd->filt.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
			}
		} else {
			CS_INT	len;
			for(used_bases=offset,i=0;i<sd->num_reads;i++,used_bases+=len){
				len=read_len[i];
				if(read_mask&(1<<i)){ /*** bind the data **/
					rlen[i]=len;
                    if(format_mask & SRA_OUTPUT_NAME_BIT) {
                        const SRAReadDesc* rd = (const SRAReadDesc*)(sd->rdesc.data);
                        lb_len[i] = (rd && rd[i].label) ? strlen(rd[i].label) : 0;
						cs_col_fmt.maxlength=150;
						cs_col_fmt.datatype=CS_CHAR_TYPE;
                        lb_ind[i]=(lb_len[i] > 0) ? sd->rdesc.ind : CS_NULLDATA;
				   		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->rdesc.outcol[i],&cs_col_fmt,(CS_BYTE*)rd[i].label,lb_len+i,lb_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                    }
					if(format_mask & SRA_OUTPUT_BASECALL_BIT){
						cs_col_fmt.maxlength=10000;
						cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
						bc_ind[i]=(rlen[i]>0)?sd->bc.ind:CS_NULLDATA;
				   		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->bc.outcol[i],&cs_col_fmt,
							(CS_BYTE*)(sd->bc.data+used_bases),rlen+i,bc_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
					}
					if(format_mask & SRA_OUTPUT_COLOR_SPACE_BIT){
                                                cs_col_fmt.maxlength=10000;
                                                cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_CHAR_TYPE:CS_TEXT_TYPE;
                                                cs_ind[i]=(rlen[i]>0)?sd->cs.ind:CS_NULLDATA;
                                                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->cs.outcol[i],&cs_col_fmt,
                                                        (CS_BYTE*)(sd->cs.data+used_bases),rlen+i,cs_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                                        }
					if(format_mask & SRA_OUTPUT_QUALITY_BIT){
						cs_col_fmt.maxlength=10000;
						cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
						qs1_ind[i]=(rlen[i]>0)?sd->qs1.ind:CS_NULLDATA;
				   		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->qs1.outcol[i],&cs_col_fmt,
							(CS_BYTE*)(sd->qs1.data+used_bases),rlen+i,qs1_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
					}
					if(format_mask & SRA_OUTPUT_QUALITY4_BIT){
						cs_col_fmt.maxlength=4*(10000);
						cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
						rlen4[i]=rlen[i]*4;
						qs4_ind[i]=(rlen[i]>0)?sd->qs4.ind:CS_NULLDATA;
				   		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->qs4.outcol[i],&cs_col_fmt,
							(CS_BYTE*)(sd->qs4.data+4*used_bases),rlen4+i,qs4_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
					}
					if(format_mask & SRA_OUTPUT_POSITION_BIT){
						cs_col_fmt.maxlength=2*(10000);
						cs_col_fmt.datatype=(cs_col_fmt.maxlength < 256)?CS_BINARY_TYPE:CS_IMAGE_TYPE;
						rlen2[i]=rlen[i]*2;
						pos_ind[i]=(rlen[i]>0)?sd->pos.ind:CS_NULLDATA;
				   		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->pos.outcol[i],&cs_col_fmt,
							(CS_BYTE*)(sd->pos.data+2*used_bases),rlen2+i,pos_ind+i)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
					}
                                        if(format_mask & SRA_OUTPUT_FILTER_BIT){
                                                cs_col_fmt.maxlength=sd->num_reads;
                                                cs_col_fmt.datatype=CS_BINARY_TYPE;
                                                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->filt.outcol[i],&cs_col_fmt,
                                                        (CS_BYTE*)(sd->filt.data?sd->filt.data+i:NULL),&filt_len,&sd->filt.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                                        }
				}
			}
		}
	}
	if(format_mask & SRA_OUTPUT_COORD_BIT){
		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->plate_outcol,&cs_plate_fmt,
			(CS_BYTE*)sd->name.data,&sd->coord.platename_len,&sd->name.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
	}
	if(format_mask & SRA_OUTPUT_SIGNAL_BIT){
		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->sig.outcol[0],&cs_sig_fmt,
			(CS_BYTE*)sd->sig.data,&sd->sig.len,&sd->sig.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
	}
	if(format_mask & SRA_OUTPUT_INTENSITY_BIT){
		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->rint.outcol[0],&cs_int_fmt,
			(CS_BYTE*)sd->rint.data,&sd->rint.len,&sd->rint.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
	}
	if(format_mask & SRA_OUTPUT_NOISE_BIT){
		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->nse.outcol[0],&cs_nse_fmt,
			(CS_BYTE*)sd->nse.data,&sd->nse.len,&sd->nse.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
	}
	if(format_mask & SRA_OUTPUT_SPOTGROUP_BIT){
		if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->sgrp.outcol[0],&cs_spotgroup_fmt,
			(CS_BYTE*)sd->sgrp.data,&sd->sgrp.len,&sd->sgrp.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
	}
	if(format_mask & SRA_OUTPUT_READ_LEN_BIT){
                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->len.outcol[0],&cs_readlen_fmt,
                        (CS_BYTE*)read_len,&read_len_len,&read_len_ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
        }
	if(format_mask & SRA_OUTPUT_READ_TYPE_BIT){
                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->trd.outcol[0],&cs_readtype_fmt,
                        (CS_BYTE*)sd->trd.data,&sd->trd.len,&sd->trd.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
        }
	if(format_mask & SRA_OUTPUT_READ_LABEL_BIT){
                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->lbl.outcol[0],&cs_lbl_fmt,
                        (CS_BYTE*)sd->lbl.data,&sd->lbl.len,&sd->lbl.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->lbls.outcol[0],&cs_lbls_fmt,
                        (CS_BYTE*)sd->lbls.data,&sd->lbls.len,&sd->lbls.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
                if(srv_bind(spp,CS_SET,SRV_ROWDATA,sd->lbll.outcol[0],&cs_lbll_fmt,
                        (CS_BYTE*)sd->lbll.data,&sd->lbll.len,&sd->lbll.ind)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
        }



	if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED) {rc=CS_FAIL; goto DONE;};
DONE:
	if(read_len && read_len_alloc) free(read_len);
	return rc;
}
CS_RETCODE
SraUserSignalSend(SRV_PROC *spp,SraUserData *sd)
{
	unsigned int	i_sig,i_ch;
	CS_INT		outcol=0,flow;
	CS_INT		rowcount=0;
	CS_INT		flowcharlen=1,reallen=sizeof(CS_REAL),smallintlen=sizeof(CS_SMALLINT),seqlen,seqleft;
	CS_SMALLINT	flowcharind=0,seqind=0;
	CS_CHAR		flowchar;
	CS_DATAFMT      cs_flow_fmt={"flow",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_flowchar_fmt={"char",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_seq_fmt={"seq",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,200,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_qs_fmt={"qual",CS_NULLTERM,CS_BINARY_TYPE,CS_FMT_UNUSED,200,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT	cs_realsig_fmt={"",CS_NULLTERM,CS_REAL_TYPE,CS_FMT_UNUSED,sizeof(CS_REAL),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT	cs_smallsig_fmt={"",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,sizeof(CS_SMALLINT),CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	union		{CS_REAL realsig[10];CS_SMALLINT shortsig[20];} sig,intensity,noise;
	struct		{char qs_str[50];} qs4[4],qs1;
	char		seq[50];
	char		*sig_ptr=NULL;
	char		*nse_ptr=NULL;
	char		*int_ptr=NULL;
	unsigned short	*pos_ptr;
	char		*bc_ptr;
	char		*qs1_ptr;
	char		*qs4_ptr;
	unsigned short pos;
	int		sigbytes=0;
	CS_RETCODE	rc=CS_SUCCEED;
	CS_INT		flow_count;


	/*** PREPARE POSITIONS ***/
	pos_ptr=(unsigned short*)sd->pos.data;

	/**** FLOW ORDER ****/
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_flow_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_flow_fmt,(CS_BYTE*)&flow,&intlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}

	/**** FLOW CHAR ****/
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_flowchar_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_flowchar_fmt,(CS_BYTE*)&flowchar,&flowcharlen,&flowcharind)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}

	
	/**** SEQUENCE ****/
	if(sd->platform==SRA_PLATFORM_ABSOLID){
		bc_ptr=(char*)sd->cs.data;
		seqleft=sd->cs.len;
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_seq_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_seq_fmt,(CS_BYTE*)seq,&seqlen,&seqind)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	} else {
		bc_ptr=(char*)sd->bc.data;
		seqleft=sd->bc.len;
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_seq_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_seq_fmt,(CS_BYTE*)seq,&seqlen,&seqind)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	}
	flow_count=(sd->flow_seq_len>0)?sd->flow_seq_len:sd->fixed_spot_len;

	/**** QUALITY1 ***/
	qs1_ptr=(char*)sd->qs1.data;
	strcpy(cs_qs_fmt.name,"qual");
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_qs_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_qs_fmt,(CS_BYTE*)&qs1,&seqlen,&seqind)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}

	if(sd->qs4.len > 0){
		/**** QUALITY4 ***/
		qs4_ptr=(char*)sd->qs4.data;
		for(i_ch=0;i_ch < 4;i_ch++){
			sprintf(cs_qs_fmt.name,"qual%s",channel_lbl[i_ch]);
			if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_qs_fmt)!=CS_SUCCEED
			   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_qs_fmt,(CS_BYTE*)(qs4+i_ch),&seqlen,&seqind)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
		}
	} else {
		qs4_ptr=NULL;
	}

	/**** SIGNAL ***/
	if(sd->sig.len > 0){
		sig_ptr=(char*)sd->sig.data;
		for(i_ch=0;i_ch < sd->num_sig_ch;i_ch++){
			switch(sd->platform){
			 case SRA_PLATFORM_ILLUMINA:
			 case SRA_PLATFORM_ABSOLID:
				sprintf(cs_realsig_fmt.name,"sig%s",
			sd->num_sig_ch<4?"":(sd->platform==SRA_PLATFORM_ABSOLID?channel_lbl_abi[i_ch]:channel_lbl[i_ch]));
				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_realsig_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_realsig_fmt,
						(CS_BYTE*)(sig.realsig+i_ch),&reallen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
				sigbytes=16;
				break;
			 case SRA_PLATFORM_454:
			 case SRA_PLATFORM_ION_TORRENT:
				sprintf(cs_smallsig_fmt.name,"sig%s",sd->num_sig_ch<4?"":channel_lbl[i_ch]);

				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_smallsig_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_smallsig_fmt,
						(CS_BYTE*)(sig.shortsig+i_ch),&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
				sigbytes=2;
				break;
			}
		}
	}
	if(sd->rint.len > 0){
		int_ptr=(char*)sd->rint.data;
		switch(sd->platform){
		 case SRA_PLATFORM_ILLUMINA:
			for(i_ch=0;i_ch < sd->num_int_ch;i_ch++){
				sprintf(cs_realsig_fmt.name,"int%s",sd->num_int_ch<4?"":channel_lbl[i_ch]);
				if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_realsig_fmt)!=CS_SUCCEED
				   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_realsig_fmt,
						(CS_BYTE*)(intensity.realsig+i_ch),&reallen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
			}
                        break;
		}
	}
	if(sd->nse.len > 0){
		nse_ptr=(char*)sd->nse.data;
                switch(sd->platform){
                 case SRA_PLATFORM_ILLUMINA:
                        for(i_ch=0;i_ch < sd->num_nse_ch;i_ch++){
                                sprintf(cs_realsig_fmt.name,"nse%s",sd->num_nse_ch<4?"":channel_lbl[i_ch]);
                                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_realsig_fmt)!=CS_SUCCEED
                                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_realsig_fmt,
                                                (CS_BYTE*)(noise.realsig+i_ch),&reallen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
                        }
                        break;
                }
        }
	seqlen=0;
	pos=pos_ptr[0];
	for(i_sig=0;i_sig < flow_count;i_sig++){
		flow=i_sig+1;
		if(sd->flow_seq_len > 0){
			flowchar=sd->flow_seq[i_sig];
		} else {
			flowcharind=-1;
		}
		if(sig_ptr){
			memcpy(&sig,sig_ptr,sigbytes);
			sig_ptr+=sigbytes;
		}
		if(int_ptr){
			memcpy(&intensity,int_ptr,16);
			int_ptr+=16;
		}
		if(nse_ptr){
                        memcpy(&noise,nse_ptr,16);
                        nse_ptr+=16;
                }


		for(seqlen=0;seqleft>0 && *pos_ptr==flow;seqlen++,pos_ptr++,bc_ptr++,seqleft--){
			seq[seqlen]=*bc_ptr;
			qs1.qs_str[seqlen]=*qs1_ptr++;
			if(qs4_ptr){
				for(i_ch=0;i_ch < 4;i_ch++,qs4_ptr++){
					qs4[i_ch].qs_str[seqlen]=*qs4_ptr;
				}
			}
		}
		if(seqlen > 0){
			seqind=0;
		} else {
			seqind=-1;
		}
		if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
		rowcount++;	
	}
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
DONE:
        return CS_SUCCEED;

}
CS_RETCODE
SraUserSequenceSend(SRV_PROC *spp,SraUserData *sd)
{
	unsigned int	i_seq,i_ch;
	CS_INT		outcol=0,base;
	CS_INT		rowcount=0,read_end;
	CS_INT		charlen=1,smallintlen=sizeof(CS_SMALLINT),seqlen;
	CS_CHAR		basechar,rtype,strand;
	CS_DATAFMT      cs_base_fmt={"base",CS_NULLTERM,CS_INT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_basechar_fmt={"char",CS_NULLTERM,CS_CHAR_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_tiny_fmt={"",CS_NULLTERM,CS_TINYINT_TYPE,CS_FMT_UNUSED,1,CS_DEF_SCALE,CS_DEF_PREC,CS_CANBENULL,0,0,NULL};
	CS_DATAFMT      cs_qs_fmt={"qual",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_smallint_fmt={"qual",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_DATAFMT      cs_pos_fmt={"pos",CS_NULLTERM,CS_SMALLINT_TYPE,CS_FMT_UNUSED,CS_UNUSED,CS_DEF_SCALE,CS_DEF_PREC,0,0,0,NULL};
	CS_SMALLINT	pos,qs4[4],qs1,frp,frw;
	CS_SMALLINT	read_num;
	CS_TINYINT	delq,delv,subq,subv,insq;
	unsigned short	*pos_ptr;
	unsigned short	*frp_ptr;
	unsigned short	*frw_ptr;
	char		*bc_ptr;
	char		*qs1_ptr;
	char		*qs4_ptr;
	char		*delq_ptr;
	char		*delv_ptr;
	char		*subq_ptr;
	char		*subv_ptr;
	char		*insq_ptr;
	char		*rtype_ptr;
	int		*rlen_ptr;
	CS_RETCODE	rc=CS_SUCCEED;


	/*** PREPARE POSITIONS ***/
	pos_ptr=(unsigned short*)sd->pos.data;

	/**** BASE ORDER ****/
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_base_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_base_fmt,(CS_BYTE*)&base,&intlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}

	/**** BASE CHAR ****/
	if(sd->platform==SRA_PLATFORM_ABSOLID){
		bc_ptr=(char*)sd->cs.data;
		seqlen=sd->cs.len;
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&basechar,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	} else {
		bc_ptr=(char*)sd->bc.data;
		seqlen=sd->bc.len;
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
		   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&basechar,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	}

	/**** POSITION  ****/
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_pos_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_pos_fmt,(CS_BYTE*)&pos,&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}


	/**** QUALITY ***/
	qs1_ptr=(char*)sd->qs1.data;
	strcpy(cs_qs_fmt.name,"qual");
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_qs_fmt)!=CS_SUCCEED
	   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_qs_fmt,(CS_BYTE*)&qs1,&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}

	if(sd->qs4.len > 0){
		/**** QUALITY4 ***/
		qs4_ptr=(char*)sd->qs4.data;
		for(i_ch=0;i_ch < 4;i_ch++){
			sprintf(cs_qs_fmt.name,"qual%s",channel_lbl[i_ch]);
			if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_qs_fmt)!=CS_SUCCEED
			   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_qs_fmt,(CS_BYTE*)(qs4+i_ch),&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
		}
	} else {
		qs4_ptr=NULL;
	}
	/**** PACBIO QUALITY ***/
	if(sd->subq.len > 0){
                /*** SUBSTITUTION QUALITY *****/
                subq_ptr=(char*)sd->subq.data;
                strcpy(cs_tiny_fmt.name,"sub_qual");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_tiny_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_tiny_fmt,(CS_BYTE*)&subq,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                subq_ptr = NULL;
        }
	if(sd->subv.len > 0){
                /*** SUBSTITUTION VALUE *****/
                subv_ptr=(char*)sd->subv.data;
                strcpy(cs_basechar_fmt.name,"sub_base");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&subv,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                subv_ptr = NULL;
        }


	if(sd->delq.len > 0){
		/*** DELETION QUALITY *****/
		delq_ptr=(char*)sd->delq.data;
		strcpy(cs_tiny_fmt.name,"del_qual");
		if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_tiny_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_tiny_fmt,(CS_BYTE*)&delq,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	} else {
		delq_ptr = NULL;
	}
	if(sd->delv.len > 0){
                /*** DELETION VALUE *****/
                delv_ptr=(char*)sd->delv.data;
                strcpy(cs_basechar_fmt.name,"del_base");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&delv,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                delv_ptr = NULL;
        }

	if(sd->insq.len > 0){
                /*** INSERTION QUALITY *****/
                insq_ptr=(char*)sd->insq.data;
                strcpy(cs_tiny_fmt.name,"ins_qual");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_tiny_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_tiny_fmt,(CS_BYTE*)&insq,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                insq_ptr = NULL;
        }

	if(sd->frp.len > 0){
                /*** PRE_BASE_FRAMES *****/
                frp_ptr=(unsigned short*)sd->frp.data;
                strcpy(cs_smallint_fmt.name,"pre_frames");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_smallint_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_smallint_fmt,(CS_BYTE*)&frp,&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                frp_ptr = NULL;
        }
	if(sd->frw.len > 0){
                /*** WIDTH_IN_FRAMES *****/
                frw_ptr=(unsigned short*)sd->frw.data;
                strcpy(cs_smallint_fmt.name,"width_frames");
                if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_smallint_fmt)!=CS_SUCCEED
                   || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_smallint_fmt,(CS_BYTE*)&frw,&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
        } else {
                frw_ptr = NULL;
        }
	/*** Read number and type ****/
	strcpy(cs_smallint_fmt.name,"read");
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_smallint_fmt)!=CS_SUCCEED
             || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_smallint_fmt,(CS_BYTE*)&read_num,&smallintlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	rtype_ptr = (char*)sd->trd.data;
	assert(rtype_ptr);
	strcpy(cs_basechar_fmt.name,"type");
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&rtype,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	strcpy(cs_basechar_fmt.name,"strand");
	if(   srv_descfmt(spp,CS_SET,SRV_ROWDATA,++outcol,&cs_basechar_fmt)!=CS_SUCCEED
           || srv_bind(spp,CS_SET,SRV_ROWDATA,outcol,&cs_basechar_fmt,(CS_BYTE*)&strand,&charlen,NULL)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
	rlen_ptr = (int*)sd->len.data;
	assert(rlen_ptr);

		
	for(i_seq=0,read_num=0,read_end=0;i_seq < seqlen;i_seq++){
		base=i_seq+1;
		basechar=*bc_ptr++;
		pos=*pos_ptr++;
		qs1=*qs1_ptr++;
		if(subq_ptr) subq=subq_ptr[i_seq];
		if(subv_ptr) subv=subv_ptr[i_seq];
		if(delq_ptr) delq=delq_ptr[i_seq];
		if(delv_ptr) delv=delv_ptr[i_seq];
		if(insq_ptr) insq=insq_ptr[i_seq];
		if(frp_ptr)  frp=frp_ptr[i_seq];
		if(frw_ptr)  frw=frw_ptr[i_seq];
		if(i_seq >= read_end){
			rtype = (rtype_ptr[read_num] & SRA_READ_TYPE_BIOLOGICAL) ? 'B':'T';
			if(rtype_ptr[read_num] & SRA_READ_TYPE_FORWARD) strand='+';
			else if(rtype_ptr[read_num] & SRA_READ_TYPE_REVERSE) strand='-';
			else strand='a';
			read_end+=rlen_ptr[read_num];
			read_num ++;
		}
		
		if(qs4_ptr){
			for(i_ch=0;i_ch < 4;i_ch++,qs4_ptr++){
				qs4[i_ch]=*((char*)qs4_ptr);
			}
		}
		if(srv_xferdata(spp,CS_SET,SRV_ROWDATA)!=CS_SUCCEED){rc=CS_FAIL;goto DONE;}
		rowcount++;	
	}
	srv_senddone(spp,SRV_DONE_MORE|SRV_DONE_COUNT,CS_TRAN_UNDEFINED,rowcount);
DONE:
        return CS_SUCCEED;
	

}
