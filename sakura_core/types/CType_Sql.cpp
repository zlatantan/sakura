#include "stdafx.h"
#include "types/CType.h"
#include "doc/CDocOutline.h"
#include "CFuncInfoArr.h"

/* PL/SQL */
void CType_Sql::InitTypeConfigImp(STypeConfig* pType)
{
	//名前と拡張子
	_tcscpy( pType->m_szTypeName, _T("PL/SQL") );
	_tcscpy( pType->m_szTypeExts, _T("sql,plsql") );

	//設定
	pType->m_cLineComment.CopyTo( 0, L"--", -1 );					/* 行コメントデリミタ */
	pType->m_cBlockComment.SetBlockCommentRule( 0, L"/*", L"*/" );	/* ブロックコメントデリミタ */
	pType->m_nStringType = 1;										/* 文字列区切り記号エスケープ方法  0=[\"][\'] 1=[""][''] */
	wcscpy( pType->m_szIndentChars, L"|★" );						/* その他のインデント対象文字 */
	pType->m_nKeyWordSetIdx[0] = 2;									/* キーワードセット */
	pType->m_nDefaultOutline = OUTLINE_PLSQL;						/* アウトライン解析方法 */
}




/*! PL/SQL関数リスト作成 */
void CDocOutline::MakeFuncList_PLSQL( CFuncInfoArr* pcFuncInfoArr )
{
	const wchar_t*	pLine;
	CLogicInt		nLineLen;
	int			i;
	int			nCharChars;
	wchar_t		szWordPrev[100];
	wchar_t		szWord[100];
	int			nWordIdx = 0;
	int			nMaxWordLeng = 70;
	int			nMode;
	wchar_t		szFuncName[80];
	CLogicInt	nFuncLine;
	int			nFuncId;
	int			nFuncNum;
	int			nFuncOrProc = 0;
	int			nParseCnt = 0;

	szWordPrev[0] = L'\0';
	szWord[nWordIdx] = L'\0';
	nMode = 0;
	nFuncNum = 0;
	CLogicInt	nLineCount;
	for( nLineCount = CLogicInt(0); nLineCount <  m_pcDocRef->m_cDocLineMgr.GetLineCount(); ++nLineCount ){
		pLine = m_pcDocRef->m_cDocLineMgr.GetLine(nLineCount)->GetDocLineStrWithEOL( &nLineLen );
		for( i = 0; i < nLineLen; ++i ){
			/* 1バイト文字だけを処理する */
			// 2005-09-02 D.S.Koba GetSizeOfChar
			nCharChars = CNativeW::GetSizeOfChar( pLine, nLineLen, i );
			if( 0 == nCharChars ){
				nCharChars = 1;
			}
//			if( 1 < nCharChars ){
//				i += (nCharChars - 1);
//				continue;
//			}
			/* シングルクォーテーション文字列読み込み中 */
			if( 20 == nMode ){
				if( L'\'' == pLine[i] ){
					if( i + 1 < nLineLen && L'\'' == pLine[i + 1] ){
						++i;
					}else{
						nMode = 0;
						continue;
					}
				}else{
				}
			}else
			/* コメント読み込み中 */
			if( 8 == nMode ){
				if( i + 1 < nLineLen && L'*' == pLine[i] &&  L'/' == pLine[i + 1] ){
					++i;
					nMode = 0;
					continue;
				}else{
				}
			}else
			/* 単語読み込み中 */
			if( 1 == nMode ){
				if( (1 == nCharChars && (
					L'_' == pLine[i] ||
					L'~' == pLine[i] ||
					(L'a' <= pLine[i] &&	pLine[i] <= L'z' )||
					(L'A' <= pLine[i] &&	pLine[i] <= L'Z' )||
					(L'0' <= pLine[i] &&	pLine[i] <= L'9' )
					) )
				 || 2 == nCharChars
				){
//					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						wmemcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = L'\0';
						nWordIdx += (nCharChars);
					}
				}
				else{
					if( 0 == nParseCnt && 0 == wcsicmp( szWord, L"FUNCTION" ) ){
						nFuncOrProc = 1;
						nParseCnt = 1;
						nFuncLine = nLineCount + CLogicInt(1);
					}
					else if( 0 == nParseCnt && 0 == wcsicmp( szWord, L"PROCEDURE" ) ){
						nFuncOrProc = 2;
						nParseCnt = 1;
						nFuncLine = nLineCount + CLogicInt(1);
					}
					else if( 0 == nParseCnt && 0 == wcsicmp( szWord, L"PACKAGE" ) ){
						nFuncOrProc = 3;
						nParseCnt = 1;
						nFuncLine = nLineCount + CLogicInt(1);
					}
					else if( 1 == nParseCnt && 3 == nFuncOrProc && 0 == wcsicmp( szWord, L"BODY" ) ){
						nFuncOrProc = 4;
						nParseCnt = 1;
					}
					else if( 1 == nParseCnt ){
						if( 1 == nFuncOrProc ||
							2 == nFuncOrProc ||
							3 == nFuncOrProc ||
							4 == nFuncOrProc ){
							++nParseCnt;
							wcscpy( szFuncName, szWord );
						}else
						if( 3 == nFuncOrProc ){

						}
					}else
					if( 2 == nParseCnt ){
						if( 0 == wcsicmp( szWord, L"IS" ) ){
							if( 1 == nFuncOrProc ){
								nFuncId = 11;	/* ファンクション本体 */
							}else
							if( 2 == nFuncOrProc ){
								nFuncId = 21;	/* プロシージャ本体 */
							}else
							if( 3 == nFuncOrProc ){
								nFuncId = 31;	/* パッケージ仕様部 */
							}else
							if( 4 == nFuncOrProc ){
								nFuncId = 41;	/* パッケージ本体 */
							}
							++nFuncNum;
							/*
							  カーソル位置変換
							  物理位置(行頭からのバイト数、折り返し無し行位置)
							  →
							  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
							*/
							CLayoutPoint ptPos;
							m_pcDocRef->m_cLayoutMgr.LogicToLayout(
								CLogicPoint(0, nFuncLine - 1),
								&ptPos
							);
							pcFuncInfoArr->AppendData( nFuncLine, ptPos.GetY2() + CLayoutInt(1), szFuncName, nFuncId );
							nParseCnt = 0;
						}
						if( 0 == wcsicmp( szWord, L"AS" ) ){
							if( 3 == nFuncOrProc ){
								nFuncId = 31;	/* パッケージ仕様部 */
								++nFuncNum;
								/*
								  カーソル位置変換
								  物理位置(行頭からのバイト数、折り返し無し行位置)
								  →
								  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
								*/
								CLayoutPoint ptPos;
								m_pcDocRef->m_cLayoutMgr.LogicToLayout(
									CLogicPoint(0, nFuncLine - 1),
									&ptPos
								);
								pcFuncInfoArr->AppendData( nFuncLine, ptPos.GetY2() + CLayoutInt(1) , szFuncName, nFuncId );
								nParseCnt = 0;
							}
							else if( 4 == nFuncOrProc ){
								nFuncId = 41;	/* パッケージ本体 */
								++nFuncNum;
								/*
								  カーソル位置変換
								  物理位置(行頭からのバイト数、折り返し無し行位置)
								  →
								  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
								*/
								CLayoutPoint ptPos;
								m_pcDocRef->m_cLayoutMgr.LogicToLayout(
									CLogicPoint(0, nFuncLine - 1),
									&ptPos
								);
								pcFuncInfoArr->AppendData( nFuncLine, ptPos.GetY2() + CLayoutInt(1) , szFuncName, nFuncId );
								nParseCnt = 0;
							}
						}
					}
					wcscpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = L'\0';
					nMode = 0;
					i--;
					continue;
				}
			}else
			/* 記号列読み込み中 */
			if( 2 == nMode ){
				if( L'_' == pLine[i] ||
					L'~' == pLine[i] ||
					(L'a' <= pLine[i] &&	pLine[i] <= L'z' )||
					(L'A' <= pLine[i] &&	pLine[i] <= L'Z' )||
					(L'0' <= pLine[i] &&	pLine[i] <= L'9' )||
					L'\t' == pLine[i] ||
					 L' ' == pLine[i] ||
					 WCODE::CR == pLine[i] ||
					 WCODE::LF == pLine[i] ||
					 L'{' == pLine[i] ||
					 L'}' == pLine[i] ||
					 L'(' == pLine[i] ||
					 L')' == pLine[i] ||
					 L';' == pLine[i] ||
					L'\'' == pLine[i] ||
					 L'/' == pLine[i] ||
					 L'-' == pLine[i]
				){
					wcscpy( szWordPrev, szWord );
					nWordIdx = 0;
					szWord[0] = L'\0';
					nMode = 0;
					i--;
					continue;
				}else{
//					++nWordIdx;
					if( nWordIdx >= nMaxWordLeng ){
						nMode = 999;
						continue;
					}else{
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						wmemcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = L'\0';
						nWordIdx += (nCharChars);
					}
				}
			}else
			/* 長過ぎる単語無視中 */
			if( 999 == nMode ){
				/* 空白やタブ記号等を飛ばす */
				if( L'\t' == pLine[i] ||
					L' ' == pLine[i] ||
					WCODE::CR == pLine[i] ||
					WCODE::LF == pLine[i]
				){
					nMode = 0;
					continue;
				}
			}else
			/* ノーマルモード */
			if( 0 == nMode ){
				/* 空白やタブ記号等を飛ばす */
				if( L'\t' == pLine[i] ||
					L' ' == pLine[i] ||
					WCODE::CR == pLine[i] ||
					WCODE::LF == pLine[i]
				){
					continue;
				}else
				if( i < nLineLen - 1 && L'-' == pLine[i] &&  L'-' == pLine[i + 1] ){
					break;
				}else
				if( i < nLineLen - 1 && L'/' == pLine[i] &&  L'*' == pLine[i + 1] ){
					++i;
					nMode = 8;
					continue;
				}else
				if( L'\'' == pLine[i] ){
					nMode = 20;
					continue;
				}else
				if( L';' == pLine[i] ){
					if( 2 == nParseCnt ){
						if( 1 == nFuncOrProc ){
							nFuncId = 10;	/* ファンクション宣言 */
						}else{
							nFuncId = 20;	/* プロシージャ宣言 */
						}
						++nFuncNum;
						/*
						  カーソル位置変換
						  物理位置(行頭からのバイト数、折り返し無し行位置)
						  →
						  レイアウト位置(行頭からの表示桁位置、折り返しあり行位置)
						*/
						CLayoutPoint ptPos;
						m_pcDocRef->m_cLayoutMgr.LogicToLayout(
							CLogicPoint(0, nFuncLine - 1),
							&ptPos
						);
						pcFuncInfoArr->AppendData( nFuncLine, ptPos.GetY2() + CLayoutInt(1) , szFuncName, nFuncId );
						nParseCnt = 0;
					}
					nMode = 0;
					continue;
				}else{
					if( (1 == nCharChars && (
						L'_' == pLine[i] ||
						L'~' == pLine[i] ||
						(L'a' <= pLine[i] &&	pLine[i] <= L'z' )||
						(L'A' <= pLine[i] &&	pLine[i] <= L'Z' )||
						(L'0' <= pLine[i] &&	pLine[i] <= L'9' )
						) )
					 || 2 == nCharChars
					){
						wcscpy( szWordPrev, szWord );
						nWordIdx = 0;

//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';
						wmemcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = L'\0';
						nWordIdx += (nCharChars);

						nMode = 1;
					}else{
						wcscpy( szWordPrev, szWord );
						nWordIdx = 0;
//						szWord[nWordIdx] = pLine[i];
//						szWord[nWordIdx + 1] = '\0';

						wmemcpy( &szWord[nWordIdx], &pLine[i], nCharChars );
						szWord[nWordIdx + nCharChars] = L'\0';
						nWordIdx += (nCharChars);

						nMode = 2;
					}
				}
			}
			i += (nCharChars - 1);
		}
	}
	return;
}



