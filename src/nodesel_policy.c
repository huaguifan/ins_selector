#define SCIP_DEBUG
/**@file   nodesel_policy.c
 * @brief  node selector using a learned policy 
 * @author He He 
 *
 * the UCT node selection rule selects the next leaf according to a mixed score of the node's actual lower bound
 *
 * @note It should be avoided to switch to uct node selection after the branch and bound process has begun because
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/
#include <assert.h>
#include <string.h>
#include "nodesel_policy.h"
#include "nodesel_oracle.h"
#include "feat.h"
#include "policy.h"
#include "struct_policy.h"
#include "scip/sol.h"
#include "scip/tree.h"
#include "scip/struct_set.h"
#include "scip/struct_scip.h"

#include "time.h"
#include "scip/prob.h"
#include "scip/type_sol.h"
#include "scip/struct_sol.h"

#define NODESEL_NAME            "policy"
#define NODESEL_DESC            "node selector which selects node according to a policy"
#define NODESEL_STDPRIORITY     10
#define NODESEL_MEMSAVEPRIORITY 0

#define DEFAULT_FILENAME        ""

/*
 * Data structures
 */

/** node selector data */
struct SCIP_NodeselData
{
   char*              polfname;           /**< name of the solution file */
   SCIP_POLICY*       policy;
   SCIP_FEAT*         feat;
};

void SCIPnodeselpolicyPrintStatistics(
   SCIP*                 scip,
   SCIP_NODESEL*         nodesel,
   FILE*                 file
   )
{
   assert(scip != NULL);
   assert(nodesel != NULL);

   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "Node selector      :\n");
   SCIPmessageFPrintInfo(scip->messagehdlr, file, 
         "  selection time   : %10.2f\n", SCIPnodeselGetTime(nodesel));
}

/** solving process initialization method of node selector (called when branch and bound process is about to begin) */
static
SCIP_DECL_NODESELINIT(nodeselInitPolicy)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /* read policy */
   SCIP_CALL( SCIPpolicyCreate(scip, &nodeseldata->policy) );
   assert(nodeseldata->polfname != NULL);
   SCIP_CALL( SCIPreadNNPolicy(scip, nodeseldata->polfname, &nodeseldata->policy) );
   // assert(nodeseldata->policy->weights != NULL); // xlm: NN policy has no weights
  
   /* create feat */
   nodeseldata->feat = NULL;
   SCIP_CALL( SCIPfeatCreate(scip, &nodeseldata->feat, SCIP_FEATNODESEL_SIZE) );
   // SCIP_CALL( SCIPhgfeatCreate(scip, &nodeseldata->feat, SCIP_FEATNODESEL_SIZE) );
   assert(nodeseldata->feat != NULL);
   SCIPfeatSetMaxDepth(nodeseldata->feat, SCIPgetNBinVars(scip) + SCIPgetNIntVars(scip));
  
   return SCIP_OKAY;
}

/** destructor of node selector to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODESELEXIT(nodeselExitPolicy)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(scip != NULL);
   assert(nodesel != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);

   assert(nodeseldata->feat != NULL);
   SCIP_CALL( SCIPfeatFree(scip, &nodeseldata->feat) );

   assert(nodeseldata->policy != NULL);
   SCIP_CALL( SCIPpolicyFree(scip, &nodeseldata->policy) );
   
   return SCIP_OKAY;
}

/** destructor of node selector to free user data (called when SCIP is exiting) */
static
SCIP_DECL_NODESELFREE(nodeselFreePolicy)
{
   SCIP_NODESELDATA* nodeseldata;
   assert(nodeseldata != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);

   SCIPfreeBlockMemory(scip, &nodeseldata);

   SCIPnodeselSetData(nodesel, NULL);

   return SCIP_OKAY;
}

/** node selection method of node selector */
static
SCIP_DECL_NODESELSELECT(nodeselSelectPolicy)
{
   SCIP_NODESELDATA* nodeseldata;
   SCIP_NODE** children;
   int nchildren;
   int nleaves;
   int nsiblings;
   int i;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);
   assert(selnode != NULL);

   nodeseldata = SCIPnodeselGetData(nodesel);
   assert(nodeseldata != NULL);

   /* collect leaves, children and siblings data */
   // SCIP_CALL( SCIPgetChildren(scip, &children, &nchildren) );
   SCIP_CALL( SCIPgetOpenNodesData(scip, NULL, &children, NULL, &nleaves, &nchildren, &nsiblings) );

   /* check newly created nodes */
   for( i = 0; i < nchildren; i++)
   {
      /* compute score */
      SCIPcalcNodeselFeat(scip, children[i], nodeseldata->feat);
      SCIPcalcNNNodeScore(children[i], nodeseldata->feat, nodeseldata->policy);

      if (TRUE)
      {
         SCIP_Real nodelowerbound = SCIPnodeGetLowerbound(children[i]);
         SCIP_Real primalbound = SCIPgetPrimalbound(scip);
         SCIP_Real dualbound = SCIPgetDualbound(scip);
         int idx = (int)SCIPnodeGetNumber(children[i]);
         SCIP_Real score = SCIPnodeGetScore(children[i]);

         SCIPdebugMessage("checking node %d pb %.3f db %.3f nlb %.3f score %f\n", idx, primalbound, dualbound, nodelowerbound, score);
      }

   }

   *selnode = SCIPgetBestNode(scip);

   if (TRUE)
   {
      int idx = -1;
      int depth = -1;
      SCIP_Real score = -1.0;
      int left = nchildren + nsiblings + nleaves;
      SCIP_Real lowerbound = -1;
      SCIP_Real dualbound = SCIPgetDualbound(scip);
      SCIP_Real primalbound = SCIPgetPrimalbound(scip);
      
      // time_t time_ptr;
      // time(&time_ptr);
      SCIP_Real bPB_time = 0.0;
      SCIP_Real current_time = SCIPgetSolvingTime(scip);
      SCIP_Real total_time = SCIPgetTotalTime(scip);

      int nsols = SCIPgetNSols(scip);
      int s;

      int stage = -1;
      if (nsols > 0)
         bPB_time = SCIPgetSolTime(scip, SCIPgetBestSol(scip));
      stage = SCIPgetStage(scip);

      if (*selnode != NULL)
      {
         idx = SCIPnodeGetNumber(*selnode);
         SCIPprintNodeRootPath(scip, *selnode, NULL);

         depth = SCIPnodeGetDepth(*selnode);
         score = SCIPnodeGetScore(*selnode);
         lowerbound = SCIPnodeGetLowerbound(*selnode);
      }

      // SCIPdebugMessage("final selecting node number %d primalbound %.3f lowerbound %.3f dualbound %.3f score %f\n", idx, primalbound, lowerbound, dualbound, score);
      SCIPdebugMessage("final selecting node number %d primalbound %.3f lowerbound %.3f dualbound %.3f time %.2f depth %d left %d bPB_time %.2f score %.3f nsols %d ", 
                                                   idx, primalbound, lowerbound, dualbound, current_time, depth, left, bPB_time, score, nsols);

      if ( *selnode != NULL)
      {
         SCIP_SOL** sols_top5;
         sols_top5 = SCIPgetSols(scip);

         for (s = 0; s < 5 && s < nsols; ++s)
         {
            SCIP_SOL* sol;
            SCIP_Real objvalue = -1.0;
            sol = sols_top5[s];

            if( SCIPsolIsOriginal(sol) )
               objvalue = SCIPsolGetOrigObj(sol);
            else
               objvalue = SCIPprobExternObjval(scip->transprob, scip->origprob, scip->set, SCIPsolGetObj(sol, scip->set, scip->transprob, scip->origprob));
            
            SCIPinfoMessage(scip, NULL, "obj%d %.3f ", s, objvalue);
         }
      }
      SCIPinfoMessage(scip, NULL, "total_time %.2f", total_time);
      SCIPinfoMessage(scip, NULL, " \n");
   
      if ( *selnode == NULL)
      {
         SCIP_SOL** sols;
         sols = SCIPgetSols(scip);

         for (s = 0; s < nsols && s < 10; ++s)
         {
            SCIP_SOL* sol;
            sol = sols[s];
            SCIPprintSol(scip, sol, NULL, FALSE);
         }
         
         SCIPdebugMessage("SCIPgetSolNodenum %lld, SCIPgetNBestSolsFound %lld\n", SCIPgetSolNodenum(scip, SCIPgetBestSol(scip)), SCIPgetNBestSolsFound(scip));
      }
   }

   if (FALSE)
      SCIPdebugMessage("Selecting node number %lld\n", *selnode != NULL ? SCIPnodeGetNumber(*selnode) : -1l);
      
   return SCIP_OKAY;
}

/** node comparison method of policy node selector */
static
SCIP_DECL_NODESELCOMP(nodeselCompPolicy)
{  /*lint --e{715}*/
   SCIP_Real score1;
   SCIP_Real score2;

   assert(nodesel != NULL);
   assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
   assert(scip != NULL);

   score1 = SCIPnodeGetScore(node1);
   score2 = SCIPnodeGetScore(node2);

   assert(score1 != 0);
   assert(score2 != 0);

   if( SCIPisGT(scip, score1, score2) )
      return -1;
   else if( SCIPisLT(scip, score1, score2) )
      return +1;
   else
   {
      if (FALSE)
      {
         SCIP_Real lowerbound1;
         SCIP_Real lowerbound2;

         assert(nodesel != NULL);
         assert(strcmp(SCIPnodeselGetName(nodesel), NODESEL_NAME) == 0);
         assert(scip != NULL);

         lowerbound1 = SCIPnodeGetLowerbound(node1);
         lowerbound2 = SCIPnodeGetLowerbound(node2);
         if( SCIPisLT(scip, lowerbound1, lowerbound2) )
            return -1;
         else if( SCIPisGT(scip, lowerbound1, lowerbound2) )
            return +1;
         else
         {
            SCIP_Real estimate1;
            SCIP_Real estimate2;

            estimate1 = SCIPnodeGetEstimate(node1);
            estimate2 = SCIPnodeGetEstimate(node2);
            if( (SCIPisInfinity(scip,  estimate1) && SCIPisInfinity(scip,  estimate2)) ||
                (SCIPisInfinity(scip, -estimate1) && SCIPisInfinity(scip, -estimate2)) ||
                SCIPisEQ(scip, estimate1, estimate2) )
            {
               SCIP_NODETYPE nodetype1;
               SCIP_NODETYPE nodetype2;

               nodetype1 = SCIPnodeGetType(node1);
               nodetype2 = SCIPnodeGetType(node2);
               if( nodetype1 == SCIP_NODETYPE_CHILD && nodetype2 != SCIP_NODETYPE_CHILD )
                  return -1;
               else if( nodetype1 != SCIP_NODETYPE_CHILD && nodetype2 == SCIP_NODETYPE_CHILD )
                  return +1;
               else if( nodetype1 == SCIP_NODETYPE_SIBLING && nodetype2 != SCIP_NODETYPE_SIBLING )
                  return -1;
               else if( nodetype1 != SCIP_NODETYPE_SIBLING && nodetype2 == SCIP_NODETYPE_SIBLING )
                  return +1;
               else
               {
                  int depth1;
                  int depth2;

                  depth1 = SCIPnodeGetDepth(node1);
                  depth2 = SCIPnodeGetDepth(node2);
                  if( depth1 < depth2 )
                     return -1;
                  else if( depth1 > depth2 )
                     return +1;
                  else
                     return 0;
               }
            }

            if( SCIPisLT(scip, estimate1, estimate2) )
               return -1;

            assert(SCIPisGT(scip, estimate1, estimate2));
            return +1;
         }
      }

      else
      {
         int depth1;
         int depth2;

         depth1 = SCIPnodeGetDepth(node1);
         depth2 = SCIPnodeGetDepth(node2);
         if( depth1 > depth2 )
            return -1;
         else if( depth1 < depth2 )
            return +1;
         else
         {
            SCIP_Real lowerbound1;
            SCIP_Real lowerbound2;

            lowerbound1 = SCIPnodeGetLowerbound(node1);
            lowerbound2 = SCIPnodeGetLowerbound(node2);
            if( SCIPisLT(scip, lowerbound1, lowerbound2) )
               return -1;
            else if( SCIPisGT(scip, lowerbound1, lowerbound2) )
               return +1;
            else
               return 0;
         }         
      }
   }
}

/*
 * node selector specific interface methods
 */

/** creates the uct node selector and includes it in SCIP */
SCIP_RETCODE SCIPincludeNodeselPolicy(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_NODESELDATA* nodeseldata;
   SCIP_NODESEL* nodesel;

   /* create policy node selector data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &nodeseldata) );

   nodesel = NULL;
   nodeseldata->polfname = NULL;

   /* use SCIPincludeNodeselBasic() plus setter functions if you want to set callbacks one-by-one and your code should
    * compile independent of new callbacks being added in future SCIP versions
    */
   SCIP_CALL( SCIPincludeNodeselBasic(scip, &nodesel, NODESEL_NAME, NODESEL_DESC, NODESEL_STDPRIORITY,
          NODESEL_MEMSAVEPRIORITY, nodeselSelectPolicy, nodeselCompPolicy, nodeseldata) );

   assert(nodesel != NULL);

   /* set non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetNodeselCopy(scip, nodesel, NULL) );
   SCIP_CALL( SCIPsetNodeselInit(scip, nodesel, nodeselInitPolicy) );
   SCIP_CALL( SCIPsetNodeselExit(scip, nodesel, nodeselExitPolicy) );
   SCIP_CALL( SCIPsetNodeselFree(scip, nodesel, nodeselFreePolicy) );

   /* add policy node selector parameters */
   SCIP_CALL( SCIPaddStringParam(scip, 
         "nodeselection/"NODESEL_NAME"/polfname",
         "name of the policy model file",
         &nodeseldata->polfname, FALSE, DEFAULT_FILENAME, NULL, NULL) );

   return SCIP_OKAY;
}
