/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sw.hxx"

#include <UndoSplitMove.hxx>

#include <doc.hxx>
#include <IDocumentUndoRedo.hxx>
#include <pam.hxx>
#include <swundo.hxx>			// fuer die UndoIds
#include <ndtxt.hxx>
#include <UndoCore.hxx>
#include <rolbck.hxx>


// MOVE

SwUndoMove::SwUndoMove( const SwPaM& rRange, const SwPosition& rMvPos )
	: SwUndo( UNDO_MOVE ), SwUndRng( rRange ),
	nMvDestNode( rMvPos.nNode.GetIndex() ),
	nMvDestCntnt( rMvPos.nContent.GetIndex() ),
    bMoveRedlines( false )
{
	bMoveRange = bJoinNext = bJoinPrev = sal_False;

	// StartNode vorm loeschen von Fussnoten besorgen!
	SwDoc* pDoc = rRange.GetDoc();
	SwTxtNode* pTxtNd = pDoc->GetNodes()[ nSttNode ]->GetTxtNode();
	SwTxtNode* pEndTxtNd = pDoc->GetNodes()[ nEndNode ]->GetTxtNode();

	pHistory = new SwHistory;

	if( pTxtNd )
	{
		pHistory->Add( pTxtNd->GetTxtColl(), nSttNode, ND_TEXTNODE );
        if ( pTxtNd->GetpSwpHints() )
        {
            pHistory->CopyAttr( pTxtNd->GetpSwpHints(), nSttNode,
                                0, pTxtNd->GetTxt().Len(), false );
        }
        if( pTxtNd->HasSwAttrSet() )
            pHistory->CopyFmtAttr( *pTxtNd->GetpSwAttrSet(), nSttNode );
	}
	if( pEndTxtNd && pEndTxtNd != pTxtNd )
	{
		pHistory->Add( pEndTxtNd->GetTxtColl(), nEndNode, ND_TEXTNODE );
        if ( pEndTxtNd->GetpSwpHints() )
        {
            pHistory->CopyAttr( pEndTxtNd->GetpSwpHints(), nEndNode,
                                0, pEndTxtNd->GetTxt().Len(), false );
        }
        if( pEndTxtNd->HasSwAttrSet() )
            pHistory->CopyFmtAttr( *pEndTxtNd->GetpSwAttrSet(), nEndNode );
	}

    pTxtNd = rMvPos.nNode.GetNode().GetTxtNode();
    if (0 != pTxtNd)
    {
		pHistory->Add( pTxtNd->GetTxtColl(), nMvDestNode, ND_TEXTNODE );
        if ( pTxtNd->GetpSwpHints() )
        {
            pHistory->CopyAttr( pTxtNd->GetpSwpHints(), nMvDestNode,
                                0, pTxtNd->GetTxt().Len(), false );
        }
        if( pTxtNd->HasSwAttrSet() )
            pHistory->CopyFmtAttr( *pTxtNd->GetpSwAttrSet(), nMvDestNode );
	}


	nFtnStt = pHistory->Count();
	DelFtn( rRange );

	if( pHistory && !pHistory->Count() )
		DELETEZ( pHistory );
}


SwUndoMove::SwUndoMove( SwDoc* pDoc, const SwNodeRange& rRg,
						const SwNodeIndex& rMvPos )
	: SwUndo( UNDO_MOVE ),
	nMvDestNode( rMvPos.GetIndex() ),
    bMoveRedlines( false )
{
	bMoveRange = sal_True;
	bJoinNext = bJoinPrev = sal_False;

	nSttCntnt = nEndCntnt = nMvDestCntnt = STRING_MAXLEN;

	nSttNode = rRg.aStart.GetIndex();
	nEndNode = rRg.aEnd.GetIndex();

//	DelFtn( rRange );

	// wird aus dem CntntBereich in den Sonderbereich verschoben ?
	sal_uLong nCntntStt = pDoc->GetNodes().GetEndOfAutotext().GetIndex();
	if( nMvDestNode < nCntntStt && rRg.aStart.GetIndex() > nCntntStt )
	{
		// loesche alle Fussnoten. Diese sind dort nicht erwuenscht.
		SwPosition aPtPos( rRg.aEnd );
		SwCntntNode* pCNd = rRg.aEnd.GetNode().GetCntntNode();
		if( pCNd )
			aPtPos.nContent.Assign( pCNd, pCNd->Len() );
		SwPosition aMkPos( rRg.aStart );
		if( 0 != ( pCNd = aMkPos.nNode.GetNode().GetCntntNode() ))
			aMkPos.nContent.Assign( pCNd, 0 );

        DelCntntIndex( aMkPos, aPtPos, nsDelCntntType::DELCNT_FTN );

		if( pHistory && !pHistory->Count() )
			DELETEZ( pHistory );
	}

	nFtnStt = 0;
}



void SwUndoMove::SetDestRange( const SwPaM& rRange,
								const SwPosition& rInsPos,
								sal_Bool bJoin, sal_Bool bCorrPam )
{
	const SwPosition *pStt = rRange.Start(),
					*pEnd = rRange.GetPoint() == pStt
						? rRange.GetMark()
						: rRange.GetPoint();

	nDestSttNode	= pStt->nNode.GetIndex();
	nDestSttCntnt	= pStt->nContent.GetIndex();
	nDestEndNode	= pEnd->nNode.GetIndex();
	nDestEndCntnt	= pEnd->nContent.GetIndex();

	nInsPosNode     = rInsPos.nNode.GetIndex();
	nInsPosCntnt	= rInsPos.nContent.GetIndex();

	if( bCorrPam )
	{
		nDestSttNode--;
		nDestEndNode--;
	}

	bJoinNext = nDestSttNode != nDestEndNode &&
				pStt->nNode.GetNode().GetTxtNode() &&
				pEnd->nNode.GetNode().GetTxtNode();
	bJoinPrev = bJoin;
}


void SwUndoMove::SetDestRange( const SwNodeIndex& rStt,
								const SwNodeIndex& rEnd,
								const SwNodeIndex& rInsPos )
{
	nDestSttNode = rStt.GetIndex();
	nDestEndNode = rEnd.GetIndex();
	if( nDestSttNode > nDestEndNode )
	{
		nDestSttNode = nDestEndNode;
		nDestEndNode = rStt.GetIndex();
	}
	nInsPosNode  = rInsPos.GetIndex();

	nDestSttCntnt = nDestEndCntnt = nInsPosCntnt = STRING_MAXLEN;
}


void SwUndoMove::UndoImpl(::sw::UndoRedoContext & rContext)
{
    SwDoc *const pDoc = & rContext.GetDoc();

	// Block, damit aus diesem gesprungen werden kann
	do {
		// erzeuge aus den Werten die Insert-Position und den Bereich
		SwNodeIndex aIdx( pDoc->GetNodes(), nDestSttNode );

		if( bMoveRange )
		{
			// nur ein Move mit SwRange
			SwNodeRange aRg( aIdx, aIdx );
			aRg.aEnd = nDestEndNode;
			aIdx = nInsPosNode;
            bool bSuccess = pDoc->MoveNodeRange( aRg, aIdx,
                    IDocumentContentOperations::DOC_MOVEDEFAULT );
            if (!bSuccess)
				break;
		}
		else
		{
			SwPaM aPam( aIdx.GetNode(), nDestSttCntnt,
						*pDoc->GetNodes()[ nDestEndNode ], nDestEndCntnt );

            // #i17764# if redlines are to be moved, we may not remove them before
            //          pDoc->Move gets a chance to handle them
            if( ! bMoveRedlines )
    			RemoveIdxFromRange( aPam, sal_False );

			SwPosition aPos( *pDoc->GetNodes()[ nInsPosNode] );
			SwCntntNode* pCNd = aPos.nNode.GetNode().GetCntntNode();
			aPos.nContent.Assign( pCNd, nInsPosCntnt );

            if( pCNd->HasSwAttrSet() )
				pCNd->ResetAllAttr();

			if( pCNd->IsTxtNode() && ((SwTxtNode*)pCNd)->GetpSwpHints() )
                ((SwTxtNode*)pCNd)->ClearSwpHintsArr( false );

			// an der InsertPos erstmal alle Attribute entfernen,
            const bool bSuccess = pDoc->MoveRange( aPam, aPos, (bMoveRedlines)
                        ? IDocumentContentOperations::DOC_MOVEREDLINES
                        : IDocumentContentOperations::DOC_MOVEDEFAULT );
            if (!bSuccess)
				break;

			aPam.Exchange();
			aPam.DeleteMark();
//			pDoc->ResetAttr( aPam, sal_False );
			if( aPam.GetNode()->IsCntntNode() )
				aPam.GetNode()->GetCntntNode()->ResetAllAttr();
			// der Pam wird jetzt aufgegeben.
		}

		SwTxtNode* pTxtNd = aIdx.GetNode().GetTxtNode();
		if( bJoinNext )
		{
			{
				RemoveIdxRel( aIdx.GetIndex() + 1, SwPosition( aIdx,
						SwIndex( pTxtNd, pTxtNd->GetTxt().Len() ) ) );
			}
			// sind keine Pams mehr im naechsten TextNode
			pTxtNd->JoinNext();
		}

		if( bJoinPrev && pTxtNd->CanJoinPrev( &aIdx ) )
		{
			// ?? sind keine Pams mehr im naechsten TextNode ??
			pTxtNd = aIdx.GetNode().GetTxtNode();
			{
				RemoveIdxRel( aIdx.GetIndex() + 1, SwPosition( aIdx,
						SwIndex( pTxtNd, pTxtNd->GetTxt().Len() ) ) );
			}
			pTxtNd->JoinNext();
		}

	} while( sal_False );

	if( pHistory )
	{
		if( nFtnStt != pHistory->Count() )
			pHistory->Rollback( pDoc, nFtnStt );
		pHistory->TmpRollback( pDoc, 0 );
		pHistory->SetTmpEnd( pHistory->Count() );
	}

	// setze noch den Cursor auf den Undo-Bereich
	if( !bMoveRange )
    {
        AddUndoRedoPaM(rContext);
    }
}


void SwUndoMove::RedoImpl(::sw::UndoRedoContext & rContext)
{
    SwPaM *const pPam = & AddUndoRedoPaM(rContext);
    SwDoc & rDoc = rContext.GetDoc();

	SwNodes& rNds = rDoc.GetNodes();
	SwNodeIndex aIdx( rNds, nMvDestNode );

	if( bMoveRange )
	{
		// nur ein Move mit SwRange
		SwNodeRange aRg( rNds, nSttNode, rNds, nEndNode );
        rDoc.MoveNodeRange( aRg, aIdx, (bMoveRedlines)
                ? IDocumentContentOperations::DOC_MOVEREDLINES
                : IDocumentContentOperations::DOC_MOVEDEFAULT );
    }
	else
	{
		SwPaM aPam( *pPam->GetPoint() );
		SetPaM( aPam );
		SwPosition aMvPos( aIdx, SwIndex( aIdx.GetNode().GetCntntNode(),
										nMvDestCntnt ));

		DelFtn( aPam );
		RemoveIdxFromRange( aPam, sal_False );

		aIdx = aPam.Start()->nNode;
		sal_Bool bJoinTxt = aIdx.GetNode().IsTxtNode();

		aIdx--;
        rDoc.MoveRange( aPam, aMvPos,
            IDocumentContentOperations::DOC_MOVEDEFAULT );

		if( nSttNode != nEndNode && bJoinTxt )
		{
			aIdx++;
			SwTxtNode * pTxtNd = aIdx.GetNode().GetTxtNode();
			if( pTxtNd && pTxtNd->CanJoinNext() )
			{
				{
					RemoveIdxRel( aIdx.GetIndex() + 1, SwPosition( aIdx,
							SwIndex( pTxtNd, pTxtNd->GetTxt().Len() ) ) );
				}
				pTxtNd->JoinNext();
			}
		}
		*pPam->GetPoint() = *aPam.GetPoint();
		pPam->SetMark();
		*pPam->GetMark() = *aPam.GetMark();
	}
}


void SwUndoMove::DelFtn( const SwPaM& rRange )
{
	// wird aus dem CntntBereich in den Sonderbereich verschoben ?
	SwDoc* pDoc = rRange.GetDoc();
	sal_uLong nCntntStt = pDoc->GetNodes().GetEndOfAutotext().GetIndex();
	if( nMvDestNode < nCntntStt &&
		rRange.GetPoint()->nNode.GetIndex() >= nCntntStt )
	{
		// loesche alle Fussnoten. Diese sind dort nicht erwuenscht.
		DelCntntIndex( *rRange.GetMark(), *rRange.GetPoint(),
                            nsDelCntntType::DELCNT_FTN );

		if( pHistory && !pHistory->Count() )
			delete pHistory, pHistory = 0;
	}
}

