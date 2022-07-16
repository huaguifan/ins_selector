/**@file   feat.c
 * @brief  methods for node features 
 * @author He He 
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "scip/def.h"
#include "feat.h"
#include "struct_feat.h"
#include "scip/tree.h"
#include "scip/var.h"
#include "scip/stat.h"
#include "scip/set.h"
#include "scip/struct_scip.h"
#include "math.h"

#include "scip/type_lp.h"
#include "scip/struct_lp.h"

/** copy feature vector value */
void SCIPfeatCopy(
   SCIP_FEAT*           feat,
   SCIP_FEAT*           sourcefeat 
   )
{
   int i;
   
   assert(feat != NULL);
   assert(feat->vals != NULL);
   assert(sourcefeat != NULL);
   assert(sourcefeat->vals != NULL);

   sourcefeat->maxdepth = feat->maxdepth;
   sourcefeat->depth = feat->depth;
   sourcefeat->size = feat->size;
   sourcefeat->boundtype = feat->boundtype;
   sourcefeat->rootlpobj = feat->rootlpobj;
   sourcefeat->sumobjcoeff = feat->sumobjcoeff;
   sourcefeat->nconstrs = feat->nconstrs;

   for( i = 0; i < feat->size; i++ )
      sourcefeat->vals[i] = feat->vals[i];
}

/** create feature vector and normalizers, initialized to zero */
SCIP_RETCODE SCIPfeatCreate(
   SCIP*                scip,
   SCIP_FEAT**          feat,
   int                  size
   )
{
   int i;

   assert(scip != NULL);
   assert(feat != NULL);
   SCIP_CALL( SCIPallocBlockMemory(scip, feat) );

   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->vals, size) );

   for( i = 0; i < size; i++ )
      (*feat)->vals[i] = 0;

   (*feat)->rootlpobj = 0;
   (*feat)->sumobjcoeff = 0;
   (*feat)->nconstrs = 0;
   (*feat)->maxdepth = 0;
   (*feat)->depth = 0;
   (*feat)->size = size;
   (*feat)->boundtype = 0;

   return SCIP_OKAY;
}

/** free feature vector */
SCIP_RETCODE SCIPfeatFree(
   SCIP*                scip,
   SCIP_FEAT**          feat 
   )
{
   assert(scip != NULL);
   assert(feat != NULL);
   assert(*feat != NULL);
   BMSfreeMemoryArray(&(*feat)->vals);
   SCIPfreeBlockMemory(scip, feat);

   return SCIP_OKAY;
}

/** calculate feature values for the node pruner of this node */
void SCIPcalcNodepruFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   )
{
   SCIP_Real lowerbound;
   SCIP_Real upperbound;
   SCIP_Bool upperboundinf;
   SCIP_Real rootlowerbound;
   SCIP_VAR* branchvar;
   SCIP_BOUNDCHG* boundchgs;
   SCIP_BRANCHDIR branchdirpreferred;
   SCIP_Real branchbound;
   SCIP_Bool haslp;
   SCIP_Real varsol;
   SCIP_Real varrootsol;

   assert(node != NULL);
   assert(SCIPnodeGetDepth(node) != 0);
   assert(feat != NULL);
   assert(feat->maxdepth != 0);

   boundchgs = node->domchg->domchgbound.boundchgs;
   assert(boundchgs != NULL);
   assert(boundchgs[0].boundchgtype == SCIP_BOUNDCHGTYPE_BRANCHING);

   feat->depth = SCIPnodeGetDepth(node);

   lowerbound = SCIPgetLowerbound(scip);
   assert(!SCIPsetIsInfinity(scip->set, lowerbound));

   upperbound = SCIPgetUpperbound(scip);
   if( SCIPsetIsInfinity(scip->set, upperbound)
      || SCIPsetIsInfinity(scip->set, -upperbound) )
      upperboundinf = TRUE;
   else
      upperboundinf = FALSE;

   rootlowerbound = REALABS(scip->stat->rootlowerbound);
   if( SCIPsetIsZero(scip->set, rootlowerbound) )
      rootlowerbound = 0.0001;
   assert(!SCIPsetIsInfinity(scip->set, rootlowerbound));

   /* currently only support branching on one variable */
   branchvar = boundchgs[0].var; 
   branchbound = boundchgs[0].newbound;
   branchdirpreferred = SCIPvarGetBranchDirection(branchvar);

   haslp = SCIPtreeHasFocusNodeLP(scip->tree);
   varsol = SCIPvarGetSol(branchvar, haslp);
   varrootsol = SCIPvarGetRootSol(branchvar);

   feat->boundtype = boundchgs[0].boundtype;

   /* calculate features */
   /* global features */
   if( SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
      feat->vals[SCIP_FEATNODEPRU_GAP] = 0;
   else if( SCIPsetIsZero(scip->set, lowerbound)
      || upperboundinf ) 
      feat->vals[SCIP_FEATNODEPRU_GAPINF] = 1;
   else
      feat->vals[SCIP_FEATNODEPRU_GAP] = (upperbound - lowerbound)/REALABS(lowerbound);

   feat->vals[SCIP_FEATNODEPRU_GLOBALLOWERBOUND] = lowerbound / rootlowerbound;
   if( upperboundinf )
      feat->vals[SCIP_FEATNODEPRU_GLOBALUPPERBOUNDINF] = 1;
   else
      feat->vals[SCIP_FEATNODEPRU_GLOBALUPPERBOUND] = upperbound / rootlowerbound;

   feat->vals[SCIP_FEATNODEPRU_NSOLUTION] = SCIPgetNSolsFound(scip);
   feat->vals[SCIP_FEATNODEPRU_PLUNGEDEPTH] = SCIPgetPlungeDepth(scip);
   feat->vals[SCIP_FEATNODEPRU_RELATIVEDEPTH] = (SCIP_Real)feat->depth / (SCIP_Real)feat->maxdepth * 10.0;

   /* node features */
   if( upperboundinf )
      upperbound = lowerbound + 0.2 * (upperbound - lowerbound);
   if( !SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
   {
      feat->vals[SCIP_FEATNODEPRU_RELATIVEBOUND] = (SCIPnodeGetLowerbound(node) - lowerbound) / (upperbound - lowerbound);
      feat->vals[SCIP_FEATNODEPRU_RELATIVEESTIMATE] = (SCIPnodeGetEstimate(node) - lowerbound)/ (upperbound - lowerbound);
   }

   /* branch var features */
   feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_BOUNDLPDIFF] = branchbound - varsol;
   feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_ROOTLPDIFF] = varrootsol - varsol;

   if( branchdirpreferred == SCIP_BRANCHDIR_DOWNWARDS )
      feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_PRIO_DOWN] = 1;
   else if(branchdirpreferred == SCIP_BRANCHDIR_UPWARDS ) 
      feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_PRIO_UP] = 1;

   feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_PSEUDOCOST] = SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol);
   /*fprintf(stderr, "%d cost: %f, varobj: %f", (int)SCIP_FEATNODEPRU_BRANCHVAR_PSEUDOCOST, SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol), varobj);*/

   feat->vals[SCIP_FEATNODEPRU_BRANCHVAR_INF] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_UPWARDS) / (SCIP_Real)feat->maxdepth : 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_DOWNWARDS) / (SCIP_Real)feat->maxdepth;
}

/** calculate feature values for the node selector of this node */
void SCIPcalcNodeselFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_FEAT*        feat
   )
{
   SCIP_NODETYPE nodetype;
   SCIP_Real nodelowerbound;
   SCIP_Real rootlowerbound;
   SCIP_Real lowerbound;            /**< global lower bound */
   SCIP_Real upperbound;           /**< global upper bound */
   SCIP_VAR* branchvar;
   SCIP_BOUNDCHG* boundchgs;
   SCIP_BRANCHDIR branchdirpreferred;
   SCIP_Real branchbound;
   SCIP_Bool haslp;
   SCIP_Bool upperboundinf;
   SCIP_Real varsol;
   SCIP_Real varrootsol;

   assert(node != NULL);
   assert(SCIPnodeGetDepth(node) != 0);
   assert(feat != NULL);
   assert(feat->maxdepth != 0);

   boundchgs = node->domchg->domchgbound.boundchgs;
   assert(boundchgs != NULL);
   assert(boundchgs[0].boundchgtype == SCIP_BOUNDCHGTYPE_BRANCHING);

   /* extract necessary information */
   nodetype = SCIPnodeGetType(node);
   nodelowerbound = SCIPnodeGetLowerbound(node);
   rootlowerbound = REALABS(scip->stat->rootlowerbound);
   if( SCIPsetIsZero(scip->set, rootlowerbound) )
      rootlowerbound = 0.0001;
   assert(!SCIPsetIsInfinity(scip->set, rootlowerbound));
   lowerbound = SCIPgetLowerbound(scip);
   upperbound = SCIPgetUpperbound(scip);
   if( SCIPsetIsInfinity(scip->set, upperbound)
      || SCIPsetIsInfinity(scip->set, -upperbound) )
      upperboundinf = TRUE;
   else
      upperboundinf = FALSE;
   feat->depth = SCIPnodeGetDepth(node);

   /* global features */
   if( SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
      feat->vals[SCIP_FEATNODESEL_GAP] = 0;
   else if( SCIPsetIsZero(scip->set, lowerbound)
      || upperboundinf ) 
      feat->vals[SCIP_FEATNODESEL_GAPINF] = 1;
   else
      feat->vals[SCIP_FEATNODESEL_GAP] = (upperbound - lowerbound)/REALABS(lowerbound);

   if( upperboundinf )
   {
      feat->vals[SCIP_FEATNODESEL_GLOBALUPPERBOUNDINF] = 1;
      /* use only 20% of the gap as upper bound */
      upperbound = lowerbound + 0.2 * (upperbound - lowerbound);
   }
   else
      feat->vals[SCIP_FEATNODESEL_GLOBALUPPERBOUND] = upperbound / rootlowerbound;

   feat->vals[SCIP_FEATNODESEL_PLUNGEDEPTH] = SCIPgetPlungeDepth(scip);
   feat->vals[SCIP_FEATNODESEL_RELATIVEDEPTH] = (SCIP_Real)feat->depth / (SCIP_Real)feat->maxdepth * 10.0;


   /* currently only support branching on one variable */
   branchvar = boundchgs[0].var; 
   branchbound = boundchgs[0].newbound;
   branchdirpreferred = SCIPvarGetBranchDirection(branchvar);

   haslp = SCIPtreeHasFocusNodeLP(scip->tree);
   varsol = SCIPvarGetSol(branchvar, haslp);
   varrootsol = SCIPvarGetRootSol(branchvar);

   feat->boundtype = boundchgs[0].boundtype;

   /* calculate features */
   feat->vals[SCIP_FEATNODESEL_LOWERBOUND] = 
      nodelowerbound / rootlowerbound;

   feat->vals[SCIP_FEATNODESEL_ESTIMATE] = 
      SCIPnodeGetEstimate(node) / rootlowerbound;

   if( !SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
      feat->vals[SCIP_FEATNODESEL_RELATIVEBOUND] = (nodelowerbound - lowerbound) / (upperbound - lowerbound);

   if( nodetype == SCIP_NODETYPE_SIBLING )
      feat->vals[SCIP_FEATNODESEL_TYPE_SIBLING] = 1;
   else if( nodetype == SCIP_NODETYPE_CHILD )
      feat->vals[SCIP_FEATNODESEL_TYPE_CHILD] = 1;
   else if( nodetype == SCIP_NODETYPE_LEAF )
      feat->vals[SCIP_FEATNODESEL_TYPE_LEAF] = 1;

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF] = branchbound - varsol;
   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF] = varrootsol - varsol;

   if( branchdirpreferred == SCIP_BRANCHDIR_DOWNWARDS )
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN] = 1;
   else if(branchdirpreferred == SCIP_BRANCHDIR_UPWARDS ) 
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP] = 1;

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST] = SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol);

   feat->vals[SCIP_FEATNODESEL_BRANCHVAR_INF] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_UPWARDS) / (SCIP_Real)feat->maxdepth : 
      SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_DOWNWARDS) / (SCIP_Real)feat->maxdepth;

   // feat 18
   feat->vals[SCIP_FEATNODESEL_BOUNDTYPE_LOWER] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 1 : 0;
   // feat 19
   feat->vals[SCIP_FEATNODESEL_BOUNDTYPE_UPPER] = 
      feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 0 : 1;
}

/*
 * Compute a bipartite graph representation of the solver 
 * calculate constraint_features, edge_features, variable_features
 */

/** create GCNN feature vector and normalizers, initialized to zero */
SCIP_RETCODE SCIPgfeatCreate(
   SCIP*                scip,
   SCIP_GFEAT**         feat,
   int                  var_size,
   int                  con_size,
   int                  edg_size
   )
{
   int i, j;
   int ncols = SCIPgetNLPCols(scip);
   int nrows = SCIPgetNLPRows(scip);

   assert(scip != NULL);
   assert(feat != NULL);
   SCIP_CALL( SCIPallocBlockMemory(scip, feat) );

   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->variable_features, ncols) );
   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->constraint_features, nrows) );
   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->edge_features, edg_size) );

   for( i = 0; i < ncols; i++ )
   {
      SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->variable_features[i], var_size));
      for ( j = 0; j < var_size; i++ )
         (*feat)->variable_features[i][j] = 0.0;
   }
   for( i = 0; i < con_size; i++ )
   {
      SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->constraint_features[i], con_size) );
      for ( j = 0; j < con_size; j++ )
         (*feat)->constraint_features[i][j] = 0.0;
   }
   for( i = 0; i < con_size; i++ ) //TODO con_size
   {
      SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->edge_features[i], edg_size) );
      for ( j = 0; j < edg_size; j++ )
         (*feat)->edge_features[i][j] = 0.0;
   }

   
   return SCIP_OKAY;
}
/** create feature vector and normalizers, initialized to zero */
SCIP_RETCODE SCIPhgfeatCreate(
   SCIP*                scip,
   SCIP_FEAT**          feat,
   int                  size
   )
{
   int i;

   assert(scip != NULL);
   assert(feat != NULL);
   SCIP_CALL( SCIPallocBlockMemory(scip, feat) );

   SCIP_ALLOC( BMSallocMemoryArray(&(*feat)->vals, size) );

   for( i = 0; i < size; i++ )
      (*feat)->vals[i] = 0;

   (*feat)->rootlpobj = 0;
   (*feat)->sumobjcoeff = 0;
   (*feat)->nconstrs = 0;
   (*feat)->maxdepth = 0;
   (*feat)->depth = 0;
   (*feat)->size = size;
   (*feat)->boundtype = 0;

   return SCIP_OKAY;
}

/** free feature vector */
SCIP_RETCODE SCIPhgfeatFree(
   SCIP*                scip,
   SCIP_FEAT**          feat 
   )
{
   assert(scip != NULL);
   assert(feat != NULL);
   assert(*feat != NULL);
   BMSfreeMemoryArray(&(*feat)->vals);
   SCIPfreeBlockMemory(scip, feat);

   return SCIP_OKAY;
}

/** calculate GCNN feature values for the node selector of this node */
void SCIPcalcNodeHeGCNNFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_HGFEAT*      feat
)
{

   SCIP_NODETYPE nodetype;
   SCIP_Real nodelowerbound;
   SCIP_Real rootlowerbound;
   SCIP_Real lowerbound;            /**< global lower bound */
   SCIP_Real upperbound;           /**< global upper bound */
   SCIP_VAR* branchvar;
   SCIP_BOUNDCHG* boundchgs;
   SCIP_BRANCHDIR branchdirpreferred;
   SCIP_Real branchbound;
   SCIP_Bool haslp;
   SCIP_Bool upperboundinf;
   SCIP_Real varsol;
   SCIP_Real varrootsol;

   /// ***                                             *///
   /* COLUMNS */
   SCIP_COL** cols = SCIPgetLPCols(scip);
   int ncols = SCIPgetNLPCols(scip);

   SCIP_SOL* sol = SCIPgetBestSol(scip);
   SCIP_VAR* var;
   SCIP_Real lb, ub, solval;
   int i, j;
   int col_i;

   /* TODO: if update */
   SCIP_Bool update = FALSE;

   int col_types[ncols];
   int col_coefs[ncols];
   int col_lbs[ncols];
   int col_ubs[ncols];
   int col_basestats[ncols];
   int col_redcosts[ncols];
   int col_ages[ncols];
   int col_solvals[ncols];
   int col_solfracs[ncols];
   int col_sol_is_at_lb[ncols];
   int col_sol_is_at_ub[ncols];
   int col_incvals[ncols];
   int col_avgincvals[ncols];

   /* ROWS */
   int nrows = SCIPgetNLPRows(scip);
   SCIP_ROW** rows = SCIPgetLPRows(scip);

   /* TODO: if update */
   SCIP_Real row_lhss[nrows];
   SCIP_Real row_rhss[nrows];
   int row_nnzrs[nrows];
   SCIP_Real row_dualsols[nrows];
   int row_basestats[nrows];
   int row_ages[nrows];
   SCIP_Real row_activities[nrows];
   SCIP_Real row_objcossims[nrows];
   SCIP_Real row_norms[nrows];
   int row_is_at_lhs[nrows];
   int row_is_at_rhs[nrows];

   int row_is_local[nrows];
   int row_is_modifiable[nrows];
   int row_is_removable[nrows];

   int nnzrs = 0;
   SCIP_Real activity, lhs, rhs, cst;
   
   SCIP_Real sim, prod;

   /* rows vars */
   SCIP_COL** row_cols;
   SCIP_Real* row_vals;

   int *coef_colidxs;
   int *coef_rowidxs;
   int *coef_vals;

   /* for L2 norm */
   int obj_norm = 0;

   /// ***                                             *///

   if( TRUE )
   {
      assert(node != NULL);
      assert(SCIPnodeGetDepth(node) != 0);
      assert(feat != NULL);
      assert(feat->maxdepth != 0);

      boundchgs = node->domchg->domchgbound.boundchgs;
      assert(boundchgs != NULL);
      assert(boundchgs[0].boundchgtype == SCIP_BOUNDCHGTYPE_BRANCHING);

      /* extract necessary information */
      nodetype = SCIPnodeGetType(node);
      nodelowerbound = SCIPnodeGetLowerbound(node);
      rootlowerbound = REALABS(scip->stat->rootlowerbound);
      if( SCIPsetIsZero(scip->set, rootlowerbound) )
         rootlowerbound = 0.0001;
      assert(!SCIPsetIsInfinity(scip->set, rootlowerbound));
      lowerbound = SCIPgetLowerbound(scip);
      upperbound = SCIPgetUpperbound(scip);
      if( SCIPsetIsInfinity(scip->set, upperbound)
         || SCIPsetIsInfinity(scip->set, -upperbound) )
         upperboundinf = TRUE;
      else
         upperboundinf = FALSE;
      feat->depth = SCIPnodeGetDepth(node);

      /* global features */
      if( SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
         feat->vals[SCIP_FEATNODESEL_GAP] = 0;
      else if( SCIPsetIsZero(scip->set, lowerbound)
         || upperboundinf ) 
         feat->vals[SCIP_FEATNODESEL_GAPINF] = 1;
      else
         feat->vals[SCIP_FEATNODESEL_GAP] = (upperbound - lowerbound)/REALABS(lowerbound);

      if( upperboundinf )
      {
         feat->vals[SCIP_FEATNODESEL_GLOBALUPPERBOUNDINF] = 1;
         /* use only 20% of the gap as upper bound */
         upperbound = lowerbound + 0.2 * (upperbound - lowerbound);
      }
      else
         feat->vals[SCIP_FEATNODESEL_GLOBALUPPERBOUND] = upperbound / rootlowerbound;

      feat->vals[SCIP_FEATNODESEL_PLUNGEDEPTH] = SCIPgetPlungeDepth(scip);
      feat->vals[SCIP_FEATNODESEL_RELATIVEDEPTH] = (SCIP_Real)feat->depth / (SCIP_Real)feat->maxdepth * 10.0;


      /* currently only support branching on one variable */
      branchvar = boundchgs[0].var; 
      branchbound = boundchgs[0].newbound;
      branchdirpreferred = SCIPvarGetBranchDirection(branchvar);

      haslp = SCIPtreeHasFocusNodeLP(scip->tree);
      varsol = SCIPvarGetSol(branchvar, haslp);
      varrootsol = SCIPvarGetRootSol(branchvar);

      feat->boundtype = boundchgs[0].boundtype;

      /* calculate features */
      feat->vals[SCIP_FEATNODESEL_LOWERBOUND] = 
         nodelowerbound / rootlowerbound;

      feat->vals[SCIP_FEATNODESEL_ESTIMATE] = 
         SCIPnodeGetEstimate(node) / rootlowerbound;

      if( !SCIPsetIsEQ(scip->set, upperbound, lowerbound) )
         feat->vals[SCIP_FEATNODESEL_RELATIVEBOUND] = (nodelowerbound - lowerbound) / (upperbound - lowerbound);

      if( nodetype == SCIP_NODETYPE_SIBLING )
         feat->vals[SCIP_FEATNODESEL_TYPE_SIBLING] = 1;
      else if( nodetype == SCIP_NODETYPE_CHILD )
         feat->vals[SCIP_FEATNODESEL_TYPE_CHILD] = 1;
      else if( nodetype == SCIP_NODETYPE_LEAF )
         feat->vals[SCIP_FEATNODESEL_TYPE_LEAF] = 1;

      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_BOUNDLPDIFF] = branchbound - varsol;
      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_ROOTLPDIFF] = varrootsol - varsol;

      if( branchdirpreferred == SCIP_BRANCHDIR_DOWNWARDS )
         feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_DOWN] = 1;
      else if(branchdirpreferred == SCIP_BRANCHDIR_UPWARDS ) 
         feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PRIO_UP] = 1;

      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_PSEUDOCOST] = SCIPvarGetPseudocost(branchvar, scip->stat, branchbound - varsol);

      feat->vals[SCIP_FEATNODESEL_BRANCHVAR_INF] = 
         feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 
         SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_UPWARDS) / (SCIP_Real)feat->maxdepth : 
         SCIPvarGetAvgInferences(branchvar, scip->stat, SCIP_BRANCHDIR_DOWNWARDS) / (SCIP_Real)feat->maxdepth;

      // feat 18
      feat->vals[SCIP_FEATNODESEL_BOUNDTYPE_LOWER] = 
         feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 1 : 0;
      // feat 19
      feat->vals[SCIP_FEATNODESEL_BOUNDTYPE_UPPER] = 
         feat->boundtype == SCIP_BOUNDTYPE_LOWER ? 0 : 1;
   }
      ////////////////////////////////////////////////////
   /* STATE vars */
   
   /* COLUMNS */
   for (i = 0; i < ncols; i++)
   {
      col_i = SCIPcolGetLPPos(cols[i]);
      var = SCIPcolGetVar(cols[i]);

      lb = SCIPcolGetLb(cols[i]);
      ub = SCIPcolGetUb(cols[i]);
      solval = SCIPcolGetPrimsol(cols[i]);

      if ( update == FALSE )
      {
         /* Varible type */
         col_types[col_i] = SCIPvarGetType(var);

         /* Objective coefficient */
         col_coefs[col_i] = SCIPcolGetObj(cols[i]);
      }

      /* Lower bound */
      if ( SCIPisInfinity(scip, REALABS(lb)) )
         col_lbs[col_i] = NAN;
      else
         col_lbs[col_i] = lb;
      
      /* Upper bound */ 
      if ( SCIPisInfinity(scip, REALABS(ub)) )
         col_ubs[col_i] = NAN;
      else
         col_ubs[col_i] = ub;
      
      /* Basis status */
      col_basestats[col_i] = SCIPcolGetBasisStatus(cols[i]);

      /* Reduced cost */
      col_redcosts[col_i] = SCIPgetColRedcost(scip, cols[i]);
      
      /* Age */
      col_ages[col_i] = cols[i]->age;
      
      /* LP solution value */
      col_solvals[col_i] = solval;
      col_solfracs[col_i] = SCIPfeasFrac(scip, solval);
      col_sol_is_at_lb[col_i] = SCIPisEQ(scip, solval, lb);
      col_sol_is_at_ub[col_i] = SCIPisEQ(scip, solval, ub);

      /* Incumbent solution value */
      if (sol == NULL)
      {
         col_incvals[col_i] = NAN;
         col_avgincvals[col_i] = NAN;
      }
      else
      {
         col_incvals[col_i] = SCIPgetSolVal(scip, sol, var);
         col_avgincvals[col_i] = SCIPvarGetAvgSol(var);
      }

   }

   if( TRUE )
   {
      /* ROWS */
      for (i = 0; i < nrows; ++i)
      {
         /* initial variables */
         row_rhss[i] = SCIP_REAL_MIN;
         row_lhss[i] = SCIP_REAL_MIN;

         /* lhs <= activity + cst <= rhs */
         lhs = SCIProwGetLhs(rows[i]);
         rhs = SCIProwGetRhs(rows[i]);
         cst = SCIProwGetConstant(rows[i]);
         activity = SCIPgetRowLPActivity(scip, rows[i]);  /* cst is part of activity */
         if (update == FALSE)
         {
            /* number of coefficients */
            row_nnzrs[i] = SCIProwGetNLPNonz(rows[i]);
            nnzrs += row_nnzrs[i];
            
            /* left-hand-side */
            if (SCIPisInfinity(scip, REALABS(lhs)))
               row_lhss[i] = NAN;
            else
               row_lhss[i] = lhs - cst;

            /* right-hand-side */
            if (SCIPisInfinity(scip, REALABS(rhs)))
               row_rhss[i] = NAN;
            else
               row_rhss[i] = rhs - cst;
            
            /* row properties */
            row_is_local[i] = SCIProwIsLocal(rows[i]);
            row_is_modifiable[i] = SCIProwIsModifiable(rows[i]);
            row_is_removable[i] = SCIProwIsRemovable(rows[i]);
            
            /* Objective cosine similarity - inspired from SCIProwGetObjParallelism()*/
            SCIPlpRecalculateObjSqrNorm(scip->set, scip->lp);
            prod = rows[i]->sqrnorm * scip->lp->objsqrnorm;
            row_objcossims[i] = SCIPisPositive(scip, prod) ? rows[i]->objprod / SQRT(prod): 0.0;
            
            /* L2 norm */
            row_norms[i] = SCIProwGetNorm(rows[i]); // cst ?
         }

         /* Dual solution */
         row_dualsols[i] = SCIProwGetDualsol(rows[i]);
         
         /* Basis status */
         row_basestats[i] = SCIProwGetBasisStatus(rows[i]);
         
         /* Age */
         row_ages[i] = SCIProwGetAge(rows[i]);
         
         /* Activity */
         row_activities[i] = activity - cst;
         
         /* Is tight */
         row_is_at_lhs[i] = SCIPisEQ(scip, activity, lhs);
         row_is_at_rhs[i] = SCIPisEQ(scip, activity, rhs);
      }

      coef_colidxs = (int*)malloc(nnzrs*sizeof(int));
      coef_rowidxs = (int*)malloc(nnzrs*sizeof(int));
      coef_vals = (int*)malloc(nnzrs*sizeof(int));

      if (update == FALSE)
      {
         j = 0;
         for (i = 0; i < nrows; ++i)
         {
            /* coefficient indexes and values */
            row_cols = SCIProwGetCols(rows[i]);
            row_vals = SCIProwGetVals(rows[i]);
            for (int k = 0; k < row_nnzrs[i]; ++k)
            {
               coef_colidxs[j+k] = SCIPcolGetLPPos(row_cols[k]);
               coef_rowidxs[j+k] = i;
               coef_vals[j+k] = row_vals[k];
            }
            j += row_nnzrs[i];
         }
      }

      /* TODO: feat内存此时应该已经申请好了，createFEAT */
      /* 1126: get features */
      for ( i = 0; i < ncols; ++i )
      {
         obj_norm += col_coefs[i] * col_coefs[i];
      }
      obj_norm = obj_norm <= 0 ? 1 : sqrt(obj_norm);
      
      /* TODO: edge_features 39984 */

      /* cal new features */
      for( i = 0; i < ncols; i++ )
      {
         feat->vals[HEGCNN_FEATNODESEL_TYPE0RATIO] += ( col_types[i] == 0 ? 1.0 : 0.0 ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_TYPE1RATIO] += ( col_types[i] == 1 ? 1.0 : 0.0 ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_TYPE2RATIO] += ( col_types[i] == 2 ? 1.0 : 0.0 ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_TYPE3RATIO] += ( col_types[i] == 3 ? 1.0 : 0.0 ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_COLHASLBRATIO] += ( SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : 1.0) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_COLHASUBRATIO] += ( SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : 1.0) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_COLSOLISATLBRATIO] += ( col_sol_is_at_lb[i] ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_COLSOLISATUBRATIO] += ( col_sol_is_at_ub[i] ) / ncols;
         feat->vals[HEGCNN_FEATNODESEL_COLAVGAGES] += col_ages[i] / ncols;
         feat->vals[HEGCNN_FEATNODESEL_ROWNNZRS] += row_nnzrs[i] / ncols;
         feat->vals[HEGCNN_FEATNODESEL_ROWAGES] += row_ages[i] / ncols;
         feat->vals[HEGCNN_FEATNODESEL_ROWISTIGHT] += ( SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : row_is_at_lhs[i] ) / ncols;
      }
   }
   
   SCIPdebugMessage("*******************************************checking node %d\n", (int)SCIPnodeGetNumber(node));
   // SCIPdebugMessage("test getState: %d %.3f %.3f %.3f\n", col_i, lb, ub, solval);
}

/** calculate GCNN feature values for the node selector of this node */
void SCIPcalcNodeGCNNFeat(
   SCIP*             scip,
   SCIP_NODE*        node,
   SCIP_GFEAT*       feat
)
{
   /* COLUMNS */
   SCIP_COL** cols = SCIPgetLPCols(scip);
   int ncols = SCIPgetNLPCols(scip);

   SCIP_SOL* sol = SCIPgetBestSol(scip);
   SCIP_VAR* var;
   SCIP_Real lb, ub, solval;
   int i, j;
   int col_i;

   /* TODO: if update */
   SCIP_Bool update = FALSE;

   int col_types[ncols];
   int col_coefs[ncols];
   int col_lbs[ncols];
   int col_ubs[ncols];
   int col_basestats[ncols];
   int col_redcosts[ncols];
   int col_ages[ncols];
   int col_solvals[ncols];
   int col_solfracs[ncols];
   int col_sol_is_at_lb[ncols];
   int col_sol_is_at_ub[ncols];
   int col_incvals[ncols];
   int col_avgincvals[ncols];

   /* ROWS */
   int nrows = SCIPgetNLPRows(scip);
   SCIP_ROW** rows = SCIPgetLPRows(scip);

   /* TODO: if update */
   SCIP_Real row_lhss[nrows];
   SCIP_Real row_rhss[nrows];
   int row_nnzrs[nrows];
   SCIP_Real row_dualsols[nrows];
   int row_basestats[nrows];
   int row_ages[nrows];
   SCIP_Real row_activities[nrows];
   SCIP_Real row_objcossims[nrows];
   SCIP_Real row_norms[nrows];
   int row_is_at_lhs[nrows];
   int row_is_at_rhs[nrows];

   int row_is_local[nrows];
   int row_is_modifiable[nrows];
   int row_is_removable[nrows];

   int nnzrs = 0;
   SCIP_Real activity, lhs, rhs, cst;
   
   SCIP_Real sim, prod;

   /* rows vars */
   SCIP_COL** row_cols;
   SCIP_Real* row_vals;

   int *coef_colidxs;
   int *coef_rowidxs;
   int *coef_vals;

   /* for L2 norm */
   int obj_norm = 0;

   /* STATE vars */
   
   /* COLUMNS */
   for (i = 0; i < ncols; i++)
   {
      col_i = SCIPcolGetLPPos(cols[i]);
      var = SCIPcolGetVar(cols[i]);

      lb = SCIPcolGetLb(cols[i]);
      ub = SCIPcolGetUb(cols[i]);
      solval = SCIPcolGetPrimsol(cols[i]);

      if ( update == FALSE )
      {
         /* Varible type */
         col_types[col_i] = SCIPvarGetType(var);

         /* Objective coefficient */
         col_coefs[col_i] = SCIPcolGetObj(cols[i]);
      }

      /* Lower bound */
      if ( SCIPisInfinity(scip, REALABS(lb)) )
         col_lbs[col_i] = NAN;
      else
         col_lbs[col_i] = lb;
      
      /* Upper bound */ 
      if ( SCIPisInfinity(scip, REALABS(ub)) )
         col_ubs[col_i] = NAN;
      else
         col_ubs[col_i] = ub;
      
      /* Basis status */
      col_basestats[col_i] = SCIPcolGetBasisStatus(cols[i]);

      /* Reduced cost */
      col_redcosts[col_i] = SCIPgetColRedcost(scip, cols[i]);
      
      /* Age */
      col_ages[col_i] = cols[i]->age;
      
      /* LP solution value */
      col_solvals[col_i] = solval;
      col_solfracs[col_i] = SCIPfeasFrac(scip, solval);
      col_sol_is_at_lb[col_i] = SCIPisEQ(scip, solval, lb);
      col_sol_is_at_ub[col_i] = SCIPisEQ(scip, solval, ub);

      /* Incumbent solution value */
      if (sol == NULL)
      {
         col_incvals[col_i] = NAN;
         col_avgincvals[col_i] = NAN;
      }
      else
      {
         col_incvals[col_i] = SCIPgetSolVal(scip, sol, var);
         col_avgincvals[col_i] = SCIPvarGetAvgSol(var);
      }

   }

   /* ROWS */
   for (i = 0; i < nrows; ++i)
   {
      /* initial variables */
      row_rhss[i] = SCIP_REAL_MIN;
      row_lhss[i] = SCIP_REAL_MIN;

      /* lhs <= activity + cst <= rhs */
      lhs = SCIProwGetLhs(rows[i]);
      rhs = SCIProwGetRhs(rows[i]);
      cst = SCIProwGetConstant(rows[i]);
      activity = SCIPgetRowLPActivity(scip, rows[i]);  /* cst is part of activity */
      if (update == FALSE)
      {
         /* number of coefficients */
         row_nnzrs[i] = SCIProwGetNLPNonz(rows[i]);
         nnzrs += row_nnzrs[i];
         
         /* left-hand-side */
         if (SCIPisInfinity(scip, REALABS(lhs)))
            row_lhss[i] = NAN;
         else
            row_lhss[i] = lhs - cst;

         /* right-hand-side */
         if (SCIPisInfinity(scip, REALABS(rhs)))
            row_rhss[i] = NAN;
         else
            row_rhss[i] = rhs - cst;
         
         /* row properties */
         row_is_local[i] = SCIProwIsLocal(rows[i]);
         row_is_modifiable[i] = SCIProwIsModifiable(rows[i]);
         row_is_removable[i] = SCIProwIsRemovable(rows[i]);
         
         /* Objective cosine similarity - inspired from SCIProwGetObjParallelism()*/
         SCIPlpRecalculateObjSqrNorm(scip->set, scip->lp);
         prod = rows[i]->sqrnorm * scip->lp->objsqrnorm;
         row_objcossims[i] = SCIPisPositive(scip, prod) ? rows[i]->objprod / SQRT(prod): 0.0;
         
         /* L2 norm */
         row_norms[i] = SCIProwGetNorm(rows[i]); // cst ?
      }

      /* Dual solution */
      row_dualsols[i] = SCIProwGetDualsol(rows[i]);
      
      /* Basis status */
      row_basestats[i] = SCIProwGetBasisStatus(rows[i]);
      
      /* Age */
      row_ages[i] = SCIProwGetAge(rows[i]);
      
      /* Activity */
      row_activities[i] = activity - cst;
      
      /* Is tight */
      row_is_at_lhs[i] = SCIPisEQ(scip, activity, lhs);
      row_is_at_rhs[i] = SCIPisEQ(scip, activity, rhs);
   }

   /* Row coefficients */
   /*
   if (update == FALSE)
   {*/
      // int coef_colidxs[nnzrs];
      // int coef_rowidxs[nnzrs];
      // int coef_vals[nnzrs];

   coef_colidxs = (int*)malloc(nnzrs*sizeof(int));
   coef_rowidxs = (int*)malloc(nnzrs*sizeof(int));
   coef_vals = (int*)malloc(nnzrs*sizeof(int));

   /*}*/
      // else
      // {
      //    coef_colidxs = prev_state['nzrcoef']['colidxs']
      //    coef_rowidxs = prev_state['nzrcoef']['rowidxs']
      //    coef_vals    = prev_state['nzrcoef']['vals']
      // }


   if (update == FALSE)
   {
      j = 0;
      for (i = 0; i < nrows; ++i)
      {
         /* coefficient indexes and values */
         row_cols = SCIProwGetCols(rows[i]);
         row_vals = SCIProwGetVals(rows[i]);
         for (int k = 0; k < row_nnzrs[i]; ++k)
         {
            coef_colidxs[j+k] = SCIPcolGetLPPos(row_cols[k]);
            coef_rowidxs[j+k] = i;
            coef_vals[j+k] = row_vals[k];
         }
         j += row_nnzrs[i];
      }
   }

   /* TODO: feat内存此时应该已经申请好了，createFEAT */
   /* 1126: get features */
   for ( i = 0; i < ncols; ++i )
   {
      obj_norm += col_coefs[i] * col_coefs[i];
   }
   obj_norm = obj_norm <= 0 ? 1 : sqrt(obj_norm);

   /* cal new features */


   /*
   for ( i = 0; i < ncols; ++i)
   {
      feat->variable_features[i][TYPE_START + col_types[i]]             = 1;
      feat->variable_features[i][COEF_NORMALIZED]                       = col_coefs[i] / obj_norm;

      feat->variable_features[i][HAS_LB]                                = col_lbs[i];
      feat->variable_features[i][HAS_UB]                                = col_ubs[i];
      feat->variable_features[i][SOL_IS_AT_LB]                          = col_sol_is_at_lb[i];
      feat->variable_features[i][SOL_IS_AT_UB]                          = col_sol_is_at_ub[i];
      feat->variable_features[i][SOL_FRAC]                              = col_solfracs[i];
      feat->variable_features[i][BASIS_STATUS_START + col_basestats[i]] = 1;
      feat->variable_features[i][REDUCED_COST]                          = col_redcosts[i] / obj_norm;
      feat->variable_features[i][AGE]                                   = col_ages[i];
      feat->variable_features[i][SOL_VAL]                               = col_solvals[i];
      feat->variable_features[i][INC_VAL]                               = col_incvals[i];
      feat->variable_features[i][AVG_INC_VAL]                           = col_avgincvals[i];   
   }
   */
   /* TODO: print nrows == 520 ? 
   for ( i = 0; i < nrows; ++i )
   {
      feat->constraint_features[i][OBJ_COSINE_SIMILARITY] = SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : -row_objcossims[i] ;
      feat->constraint_features[i][OBJ_COSINE_SIMILARITY] = SCIPisEQ(scip, row_rhss[i], SCIP_REAL_MIN) ? 0.0 : +row_objcossims[i] ;
      feat->constraint_features[i][BIAS] = SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : - (row_lhss[i] / row_norms[i]);
      feat->constraint_features[i][BIAS] = SCIPisEQ(scip, row_rhss[i], SCIP_REAL_MIN) ? 0.0 : + (row_rhss[i] / row_norms[i]);
      feat->constraint_features[i][IS_TIGHT] = SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : row_is_at_lhs[i];
      feat->constraint_features[i][IS_TIGHT] = SCIPisEQ(scip, row_rhss[i], SCIP_REAL_MIN) ? 0.0 : row_is_at_rhs[i];
      feat->constraint_features[i][AGE_0] = SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : row_ages[i] / SCIPgetNLPs(scip);
      feat->constraint_features[i][AGE_0] = SCIPisEQ(scip, row_rhss[i], SCIP_REAL_MIN) ? 0.0 : row_ages[i] / SCIPgetNLPs(scip);
      feat->constraint_features[i][DUALSOL_VAL_NORMALIZED] = SCIPisEQ(scip, row_lhss[i], SCIP_REAL_MIN) ? 0.0 : - (row_dualsols[i] / (row_norms[i] * obj_norm));
      feat->constraint_features[i][DUALSOL_VAL_NORMALIZED] = SCIPisEQ(scip, row_rhss[i], SCIP_REAL_MIN) ? 0.0 : + (row_dualsols[i] / (row_norms[i] * obj_norm));
   }*/

   /* TODO: edge_features 39984 */
   
   SCIPdebugMessage("*******************************************checking node %d\n", (int)SCIPnodeGetNumber(node));
   SCIPdebugMessage("test getState: %d %.3f %.3f %.3f\n", col_i, lb, ub, solval);
}

void SCIPfeatNNPrint(
   SCIP*             scip,
   FILE*             file,    /* trj file */
   FILE*             wfile,   /* unknown */
   SCIP_FEAT*        feat,   /* not know feat type */
   int               label,
   SCIP_Bool         negate
)
{
   int size;
   int i;
   // int offset;

   assert(scip != NULL);
   assert(feat != NULL);
   assert(feat->depth != 0);

   // offset = SCIPfeatGetOffset(feat); /* 0 or 18 */
   size = SCIPfeatGetSize(feat);

   /* SCIPinfoMessage(scip, NULL, "*******************************feat size: %i, depth: %i, tmp: %d, boundtype: %i, offset: %d \n",\
   //                  feat->size, feat->depth, (feat->maxdepth / 10), (int)(feat->boundtype), offset);*/
   

   SCIPinfoMessage(scip, file, "%d ", label);  

   for( i = 0; i < size; i++ )
   {
      SCIPinfoMessage(scip, file, "%d:%f ", i + 1, feat->vals[i]);
   }

   SCIPinfoMessage(scip, file, "\n");
}

void SCIPfeatDiffNNPrintConcat(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat1,
   SCIP_FEAT*        feat2,
   SCIP_FEAT*        left_feat,
   SCIP_FEAT*        right_feat,
   int               label,
   SCIP_Bool         negate
   )
{
   int size;
   int i;

   assert(scip != NULL);
   assert(feat1 != NULL);
   assert(feat2 != NULL);
   assert(feat1->depth != 0);
   assert(feat2->depth != 0);
   assert(feat1->size == feat2->size);
   assert(feat1->size == left_feat->size);
   assert(feat1->size == right_feat->size);

   size = SCIPfeatGetSize(feat1);

   SCIPinfoMessage(scip, file, "%d ", label);
   
   /* left and right */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 1, left_feat->vals[i]);
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 21, right_feat->vals[i]);
   /* feat1 */ 
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 41, feat1->vals[i]);
   
   /* (left+right)/2 */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 61, left_feat->vals[i]);
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 81, right_feat->vals[i]);
   /* feat2 */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 101, feat2->vals[i]);

   SCIPinfoMessage(scip, file, "\n");
}

/** write feature vector single node in libsvm format */
void SCIPfeatSingleNNPrint(
   SCIP*             scip,
   FILE*             file,
   SCIP_FEAT*        feat,
   int               node_idx,
   int               group_idx
   )
{
   int size;
   int i;

   assert(scip != NULL);
   assert(feat != NULL);
   assert(feat->depth != 0);

   size = SCIPfeatGetSize(feat);
   
   /* node idx */
   SCIPinfoMessage(scip, file, "%d:%d ", 1, node_idx);
   
   /* group idx */
   SCIPinfoMessage(scip, file, "%d:%d ", 2, group_idx);
   
   /* feat */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 3, feat->vals[i]);

   SCIPinfoMessage(scip, file, "\n");
}

/** write feature vector diff (feat1 & feat2) in libsvm format */
void SCIPfeatDiffNNPrint(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat1,
   SCIP_FEAT*        feat2,
   int               label,
   SCIP_Bool         negate
   )
{
   int size;
   int i;

   assert(scip != NULL);
   assert(feat1 != NULL);
   assert(feat2 != NULL);
   assert(feat1->depth != 0);
   assert(feat2->depth != 0);
   assert(feat1->size == feat2->size);

   size = SCIPfeatGetSize(feat1);

   // if( negate )
   // {
   //    SCIP_FEAT* tmp = feat1;
   //    feat1 = feat2;
   //    feat2 = tmp;
   //    label = -1 * label;
   // }

   SCIPinfoMessage(scip, file, "%d ", label);
   
   /* feat1 */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 1, feat1->vals[i]);
   /* -feat2 */
   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + 1, feat2->vals[i]);

   SCIPinfoMessage(scip, file, "\n");
}

/** write feature vector diff (feat1 - feat2) in libsvm format */
void SCIPfeatDiffLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat1,
   SCIP_FEAT*        feat2,
   int               label,
   SCIP_Bool         negate
   )
{
   int size;
   int i;
   int offset1;
   int offset2;
   SCIP_Real weight;

   assert(scip != NULL);
   assert(feat1 != NULL);
   assert(feat2 != NULL);
   assert(feat1->depth != 0);
   assert(feat2->depth != 0);
   assert(feat1->size == feat2->size);

   weight = SCIPfeatGetWeight(feat1);
   SCIPinfoMessage(scip, wfile, "%f\n", weight);  

   if( negate )
   {
      SCIP_FEAT* tmp = feat1;
      feat1 = feat2;
      feat2 = tmp;
      label = -1 * label;
   }

   size = SCIPfeatGetSize(feat1);
   offset1 = SCIPfeatGetOffset(feat1);
   offset2 = SCIPfeatGetOffset(feat2);

   SCIPinfoMessage(scip, file, "%d ", label);

   if( offset1 == offset2 )
   {
      for( i = 0; i < size; i++ )
         SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i] - feat2->vals[i]);
   }
   else
   {
      /* libsvm requires sorted indices, write smaller indices first */
      if( offset1 < offset2 )
      {
         /* feat1 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i]);
         /* -feat2 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset2 + 1, -feat2->vals[i]);
      }
      else
      {
         /* -feat2 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset2 + 1, -feat2->vals[i]);
         /* feat1 */
         for( i = 0; i < size; i++ )
            SCIPinfoMessage(scip, file, "%d:%f ", i + offset1 + 1, feat1->vals[i]);
      }
   }

   SCIPinfoMessage(scip, file, "\n");
}

/** write feature vector in libsvm format(only for prune) */
void SCIPfeatLIBSVMPrint(
   SCIP*             scip,
   FILE*             file,
   FILE*             wfile,
   SCIP_FEAT*        feat,
   int               label
   )
{
   int size;
   int i;
   int offset;
   SCIP_Real weight;

   assert(scip != NULL);
   assert(feat != NULL);
   assert(feat->depth != 0);

   weight = SCIPfeatGetWeight(feat);
   SCIPinfoMessage(scip, wfile, "%f\n", weight);  

   size = SCIPfeatGetSize(feat);
   offset = SCIPfeatGetOffset(feat);

   SCIPinfoMessage(scip, file, "%d ", label);  

   for( i = 0; i < size; i++ )
      SCIPinfoMessage(scip, file, "%d:%f ", i + offset + 1, feat->vals[i]);

   SCIPinfoMessage(scip, file, "\n");
}


/*
 * simple functions implemented as defines
 */

/* In debug mode, the following methods are implemented as function calls to ensure
 * type validity.
 * In optimized mode, the methods are implemented as defines to improve performance.
 * However, we want to have them in the library anyways, so we have to undef the defines.
 */

#undef SCIPfeatGetSize
#undef SCIPfeatGetVals
#undef SCIPfeatGetOffset
#undef SCIPfeatSetRootlpObj
#undef SCIPfeatSetSumObjCoeff
#undef SCIPfeatSetMaxDepth
#undef SCIPfeatSetNConstrs

void SCIPfeatSetRootlpObj(
   SCIP_FEAT*    feat,
   SCIP_Real     rootlpobj
   )
{
   assert(feat != NULL);

   feat->rootlpobj = rootlpobj;
}

SCIP_Real* SCIPfeatGetVals(
   SCIP_FEAT*    feat 
   )
{
   assert(feat != NULL);

   return feat->vals;
}

int SCIPfeatGetSize(
   SCIP_FEAT*    feat 
   )
{
   assert(feat != NULL);

   return feat->size;
}

void SCIPfeatSetSumObjCoeff(
   SCIP_FEAT*    feat,
   SCIP_Real     sumobjcoeff
   )
{
   assert(feat != NULL);

   feat->sumobjcoeff = sumobjcoeff;
}

void SCIPfeatSetMaxDepth(
   SCIP_FEAT*    feat,
   int           maxdepth
   )
{
   assert(feat != NULL);

   feat->maxdepth = maxdepth;
}

void SCIPfeatSetNConstrs(
   SCIP_FEAT*    feat,
   int           nconstrs 
   )
{
   assert(feat != NULL);

   feat->nconstrs = nconstrs;
}

/** returns the weight of the example */
SCIP_Real SCIPfeatGetWeight(
   SCIP_FEAT* feat
   )
{
   assert(feat != NULL);
   /* depth=1 => weight = 5; depth=0.6*maxdepth => weight = 1 */
   return 5 * exp(-(feat->depth-1) / (0.6*feat->maxdepth) * 1.61);
}

int SCIPfeatGetOffset(
   SCIP_FEAT* feat
   )
{
   assert(feat != NULL);
   return (feat->size * 2) * (feat->depth / (feat->maxdepth / 10)) + (feat->size * (int)feat->boundtype);
}

