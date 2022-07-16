/**@file   type_feat.h
 * @ingroup TYPEDEFINITIONS
 * @brief  type definitions for node features 
 * @author He He 
 *
 *  This file defines the interface for node feature implemented in C.
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_TYPE_FEAT_H__
#define __SCIP_TYPE_FEAT_H__

#ifdef __cplusplus
extern "C" {
#endif

/** feature type */
enum SCIP_FeatType
{
   SCIP_FEATTYPE_NODESEL = 0,
   SCIP_FEATTYPE_NODEPRU = 1
};
typedef enum SCIP_FeatType SCIP_FEATTYPE; 

/** node selector features */
/** features are respective to the depth and the branch direction */
/* TODO: remove inf; scale of objconstr is off; add relative bounds to parent node? */
enum SCIP_FeatNodesel
{
   SCIP_FEATNODESEL_LOWERBOUND               = 0,
   SCIP_FEATNODESEL_ESTIMATE                 = 1,
   SCIP_FEATNODESEL_TYPE_SIBLING             = 2,
   SCIP_FEATNODESEL_TYPE_CHILD               = 3,
   SCIP_FEATNODESEL_TYPE_LEAF                = 4,
   SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF    = 5,
   SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF     = 6,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP        = 7,
   SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN      = 8,
   SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST     = 9,
   SCIP_FEATNODESEL_BRANCHVAR_INF            = 10,
   SCIP_FEATNODESEL_RELATIVEBOUND            = 11,
   SCIP_FEATNODESEL_GLOBALUPPERBOUND         = 12, 
   SCIP_FEATNODESEL_GAP                      = 13,
   SCIP_FEATNODESEL_GAPINF                   = 14,
   SCIP_FEATNODESEL_GLOBALUPPERBOUNDINF      = 15,
   SCIP_FEATNODESEL_PLUNGEDEPTH              = 16,
   SCIP_FEATNODESEL_RELATIVEDEPTH            = 17,

   SCIP_FEATNODESEL_BOUNDTYPE_LOWER          = 18,
   SCIP_FEATNODESEL_BOUNDTYPE_UPPER          = 19
};
typedef enum SCIP_FeatNodesel SCIP_FEATNODESEL;     /**< feature of node */

/** node pruner features */
/** features are respective to the depth and the branch direction */
enum SCIP_FeatNodepru
{
   SCIP_FEATNODEPRU_GLOBALLOWERBOUND         = 0,
   SCIP_FEATNODEPRU_GLOBALUPPERBOUND         = 1, 
   SCIP_FEATNODEPRU_GAP                      = 2,
   SCIP_FEATNODEPRU_NSOLUTION                = 3,
   SCIP_FEATNODEPRU_PLUNGEDEPTH              = 4,
   SCIP_FEATNODEPRU_RELATIVEDEPTH            = 5,
   SCIP_FEATNODEPRU_RELATIVEBOUND            = 6,
   SCIP_FEATNODEPRU_RELATIVEESTIMATE         = 7,
   SCIP_FEATNODEPRU_GAPINF                   = 8,
   SCIP_FEATNODEPRU_GLOBALUPPERBOUNDINF      = 9, 
   SCIP_FEATNODEPRU_BRANCHVAR_BOUNDLPDIFF    = 10,
   SCIP_FEATNODEPRU_BRANCHVAR_ROOTLPDIFF     = 11,
   SCIP_FEATNODEPRU_BRANCHVAR_PRIO_UP        = 12,
   SCIP_FEATNODEPRU_BRANCHVAR_PRIO_DOWN      = 13,
   SCIP_FEATNODEPRU_BRANCHVAR_PSEUDOCOST     = 14,
   SCIP_FEATNODEPRU_BRANCHVAR_INF            = 15 
};
typedef enum SCIP_FeatNodepru SCIP_FEATNODEPRU;     /**< feature of node */
typedef struct SCIP_Feat SCIP_FEAT;

/* Varible features */
enum SCIP_Feat_Var
{
   TYPE_0           = 0,    
   TYPE_1           = 1,    
   TYPE_2           = 2,    
   TYPE_3           = 3,    
   COEF_NORMALIZED  = 4,             
   HAS_LB           = 5,          
   HAS_UB           = 6,       
   SOL_IS_AT_LB     = 7,       
   SOL_IS_AT_UB     = 8,       
   SOL_FRAC         = 9,    
   BASIS_STATUS_0   = 10,       
   BASIS_STATUS_1   = 11,       
   BASIS_STATUS_2   = 12,       
   BASIS_STATUS_3   = 13,       
   REDUCED_COST     = 14,   // TODO delete       
   AGE              = 15,       
   SOL_VAL          = 16,    
   INC_VAL          = 17,    
   AVG_INC_VAL      = 18        
};
typedef enum SCIP_Feat_Var SCIP_FEAT_VAR;

/* Constraint features */
enum SCIP_Feat_Cons
{
   OBJ_COSINE_SIMILARITY    = 0,  
   BIAS                     = 1,     
   IS_TIGHT                 = 2,     
   AGE_0                    = 3,     
   DUALSOL_VAL_NORMALIZED   = 4     
};
typedef enum SCIP_Feat_Cons SCIP_FEAT_CONS;

typedef struct SCIP_GFeat SCIP_GFEAT;

enum SCIP_Feat_HEGCNN
{
   HEGCNN_FEATNODESEL_LOWERBOUND               = 0,
   HEGCNN_FEATNODESEL_ESTIMATE                 = 1,
   HEGCNN_FEATNODESEL_TYPE_SIBLING             = 2,
   HEGCNN_FEATNODESEL_TYPE_CHILD               = 3,
   HEGCNN_FEATNODESEL_TYPE_LEAF                = 4,
   HEGCNN_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF    = 5,
   HEGCNN_FEATNODESEL_BRANCHVAR_ROOTLPDIFF     = 6,
   HEGCNN_FEATNODESEL_BRANCHVAR_PRIO_UP        = 7,
   HEGCNN_FEATNODESEL_BRANCHVAR_PRIO_DOWN      = 8,
   HEGCNN_FEATNODESEL_BRANCHVAR_PSEUDOCOST     = 9,
   HEGCNN_FEATNODESEL_BRANCHVAR_INF            = 10,
   HEGCNN_FEATNODESEL_RELATIVEBOUND            = 11,
   HEGCNN_FEATNODESEL_GLOBALUPPERBOUND         = 12, 
   HEGCNN_FEATNODESEL_GAP                      = 13,
   HEGCNN_FEATNODESEL_GAPINF                   = 14,
   HEGCNN_FEATNODESEL_GLOBALUPPERBOUNDINF      = 15,
   HEGCNN_FEATNODESEL_PLUNGEDEPTH              = 16,
   HEGCNN_FEATNODESEL_RELATIVEDEPTH            = 17,
   HEGCNN_FEATNODESEL_BOUNDTYPE_LOWER          = 18,
   HEGCNN_FEATNODESEL_BOUNDTYPE_UPPER          = 19,  ////------GCNN 
   HEGCNN_FEATNODESEL_TYPE0RATIO               = 20,                
   HEGCNN_FEATNODESEL_TYPE1RATIO               = 21,                      
   HEGCNN_FEATNODESEL_TYPE2RATIO               = 22,             
   HEGCNN_FEATNODESEL_TYPE3RATIO               = 23,                   
   HEGCNN_FEATNODESEL_COLHASLBRATIO            = 24,
   HEGCNN_FEATNODESEL_COLHASUBRATIO            = 25,       
   HEGCNN_FEATNODESEL_COLSOLISATLBRATIO        = 26,
   HEGCNN_FEATNODESEL_COLSOLISATUBRATIO        = 27,
   HEGCNN_FEATNODESEL_COLAVGAGES               = 28,
   HEGCNN_FEATNODESEL_ROWNNZRS                 = 29,
   HEGCNN_FEATNODESEL_ROWAGES                  = 30,
   HEGCNN_FEATNODESEL_ROWISTIGHT               = 31
};
   // HEGCNN_FEATNODESEL_ROWISAT_LHS            = 31,                      
   // HEGCNN_FEATNODESEL_ROWISAT_RHS            = 32,       
   // HEGCNN_FEATNODESEL_ROWISLOCAL             = 33,          
   // HEGCNN_FEATNODESEL_ROWISMODIFIABLE        = 34,    
   // HEGCNN_FEATNODESEL_ROWISREMOVABLE         = 35,       
typedef struct SCIP_HGFeat SCIP_HGFEAT;

#define SCIP_FEATNODESEL_SIZE 20
#define SCIP_FEATNODEPRU_SIZE 16 

#define SCIP_FEATVAR_SIZE 19
#define SCIP_FEATCON_SIZE 5
#define SCIP_FEATEDG_SIZE 1

#define SCIP_FEATHEGCNN_SIZE 25

#define TYPE_START 0
#define BASIS_STATUS_START 10

#ifdef __cplusplus
}
#endif

#endif
