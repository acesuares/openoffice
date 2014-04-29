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



#ifndef INCLUDED_TABLE_MANAGER_HXX
#define INCLUDED_TABLE_MANAGER_HXX

#include <resourcemodel/TableData.hxx>
#include <resourcemodel/WW8ResourceModel.hxx>
#include <doctok/sprmids.hxx>

#include <boost/shared_ptr.hpp>
#include <stack>
#include "util.hxx"
#include "TagLogger.hxx"

namespace writerfilter
{

using namespace ::std;

template <typename T, typename PropertiesPointer>
/**
   Class to handle events generated by TableManager::resolveCurrentTable
 */
class WRITERFILTER_DLLPUBLIC TableDataHandler
{
public:
    typedef boost::shared_ptr<TableDataHandler> Pointer_t;

    /**
       Handle start of table.

       @param nRows   number of rows in the table
       @param nDepth  depth of the table in surrounding table hierarchy
       @param pProps  properties of the table
     */
    virtual void startTable(
        unsigned int nRows,
        unsigned int nDepth,
        PropertiesPointer pProps ) = 0;

    /**
       Handle end of table.
     */
    virtual void endTable(
        const unsigned int nDepth ) = 0;

    /**
       Handle start of row.

       @param nCols    number of columns in the table
       @param pProps   properties of the row
     */
    virtual void startRow(
        unsigned int nCols,
        PropertiesPointer pProps ) = 0;

    /**
       Handle end of row.
    */
    virtual void endRow() = 0;

    /**
       Handle start of cell.

       @param rT     start handle of the cell
       @param pProps properties of the cell
    */
    virtual void startCell(
        const T & rT,
        PropertiesPointer pProps ) = 0;

    /**
        Handle end of cell.

        @param rT    end handle of cell
    */
    virtual void endCell(
        const T & rT ) = 0;
};

template <typename T, typename PropertiesPointer>
/**
   The table manager.

   This class gets forwarded events from the tokenizer. It gathers the
   table data and after ending the table generates events for the
   table structure. The events have to be handles by a TableDataHandler.

 */
class TableManager
{
#ifdef DEBUG_TABLE
    TagLogger::Pointer_t mpTableLogger;
#endif    

    class TableManagerState
    {
        /**
         properties at the current point in document
         */
        PropertiesPointer mpProps;
        
        /**
         properties of the current cell
         */
        PropertiesPointer mpCellProps;
        
        /**
         properties of the current row
         */
        PropertiesPointer mpRowProps;
        
        /**
         properties of the current table
         */
        stack<PropertiesPointer> mTableProps;
        
        /**
         true if at the end of a row
         */
        bool mbRowEnd;
        
        /**
         true when in a cell
         */
        bool mbInCell;
        
        /**
         true when at the end of a cell
         */
        bool mbCellEnd; 
		
    public:
		/**
		 Constructor
		 */
		TableManagerState()
		: mbRowEnd(false), mbInCell(false), mbCellEnd(false)
		{
		}
        
        virtual ~TableManagerState()
        {
        }
        
        void startLevel()
        {
            PropertiesPointer pProps;
            mTableProps.push(pProps);
        }
        
        void endLevel()
        {
            mTableProps.pop();
        }

        /**
         Reset to initial state at beginning of row.
         */
        void resetCellSpecifics()
        {
            mbRowEnd = false;
            mbInCell = false;
            mbCellEnd = false;
        }

        void resetProps()
        {
            mpProps.reset();
        }

        void setProps(PropertiesPointer pProps)
        {
            mpProps = pProps;
        }

        PropertiesPointer getProps()
        {
            return mpProps;
        }

        void resetCellProps()
        {
            mpCellProps.reset();
        }

        void setCellProps(PropertiesPointer pProps)
        {
            mpCellProps = pProps;
        }

        PropertiesPointer getCellProps()
        {
            return mpCellProps;
        }

        void resetRowProps()
        {
            mpRowProps.reset();
        }

        void setRowProps(PropertiesPointer pProps)
        {
            mpRowProps = pProps;
        }

        PropertiesPointer getRowProps()
        {
            return mpRowProps;
        }

        void resetTableProps()
        {
            if (mTableProps.size() > 0)
                mTableProps.top().reset();
        }

        void setTableProps(PropertiesPointer pProps)
        {
            if (mTableProps.size() > 0)
                mTableProps.top() = pProps;
        }

        PropertiesPointer getTableProps()
        {
            PropertiesPointer pResult;
            
            if (mTableProps.size() > 0)
                pResult = mTableProps.top();
            
            return pResult;
        }

        void setInCell(bool bInCell)
        {
            mbInCell = bInCell;
        }

        bool isInCell() const
        {
            return mbInCell;
        }

        void setCellEnd(bool bCellEnd)
        {
            mbCellEnd = bCellEnd;
        }

        bool isCellEnd() const
        {
            return mbCellEnd;
        }

        void setRowEnd(bool bRowEnd)
        {
            mbRowEnd = bRowEnd;
        }

        bool isRowEnd() const
        {
            return mbRowEnd;
        }
    };

    /**
     handle for the current position in document
     */
    T mCurHandle;

    TableManagerState mState;

protected:
    PropertiesPointer getProps()
    {
        return mState.getProps();
    }

    void setProps(
        PropertiesPointer pProps )
    {
        mState.setProps( pProps );
    }

    void resetProps()
    {
        mState.resetProps();
    }

    PropertiesPointer getCellProps()
    {
        return mState.getCellProps();
    }

    void setCellProps(
        PropertiesPointer pProps )
    {
        mState.setCellProps( pProps );
    }

    void resetCellProps()
    {
        mState.resetCellProps();
    }

    PropertiesPointer getRowProps()
    {
        return mState.getRowProps();
    }

    void setRowProps(
        PropertiesPointer pProps )
    {
        mState.setRowProps( pProps );
    }

    void resetRowProps()
    {
        mState.resetRowProps();
    }

    void setInCell(
        bool bInCell )
    {
        mState.setInCell( bInCell );
    }

    bool isInCell() const
    {
        return mState.isInCell();
    }

    void setCellEnd(
        bool bCellEnd )
    {
        mState.setCellEnd( bCellEnd );
    }

    bool isCellEnd() const
    {
        return mState.isCellEnd();
    }

    void setRowEnd(
        bool bRowEnd )
    {
        mState.setRowEnd( bRowEnd );
    }

    bool isRowEnd() const
    {
        return mState.isRowEnd();
    }

    PropertiesPointer getTableProps()
    {
        return mState.getTableProps();
    }

    void setTableProps(
        PropertiesPointer pProps )
    {
        mState.setTableProps( pProps );
    }

    void resetTableProps()
    {
        mState.resetTableProps();
    }

    T getHandle()
    {
        return mCurHandle;
    }

    void setHandle(
        const T & rHandle )
    {
        mCurHandle = rHandle;
    }

private:
    typedef boost::shared_ptr<T> T_p;

    /**
       depth of the current cell
    */
    sal_uInt32 mnTableDepthNew;

    /**
        depth of the previous cell
    */
    sal_uInt32 mnTableDepth;

    /**
       stack of table data

       for each level of nested tables there is one frame in the stack
     */
    stack<typename TableData<T, PropertiesPointer>::Pointer_t > mTableDataStack;

    typedef typename TableDataHandler<T, PropertiesPointer>::Pointer_t TableDataHandlerPointer_t;

    /**
       handler for resolveCurrentTable
     */
    TableDataHandlerPointer_t mpTableDataHandler;

    /**
       Set flag which indicates the current handle is in a cell.
     */
    void inCell();

    /**
       Set flag which indicate the current handle is at the end of a cell.
    */
    void endCell();

    /**
       Set the table depth of the current cell.

       @param nDepth     the cell depth
     */
    void cellDepth(sal_uInt32 nDepth);

    /**
       Set flag indication the current handle is at the end of a row.
    */
    void endRow();

    /**
       Resolve the current table to the TableDataHandler.
     */
    void resolveCurrentTable();
    
    /**
     Open a cell at current level.
     */
     
    void openCell(const T & handle, PropertiesPointer pProps);
     
    /**
     Close a cell at current level.
     */
    void closeCell(const T & handle);
    
    /**
     Ensure a cell is open at the current level.
    */
    void ensureOpenCell(PropertiesPointer pProps);

protected:

    /**
       Return current table depth.
     */
    sal_uInt32 getTableDepthNew() { return mnTableDepthNew; }

    /**
       Action to be carried out at the end of the last paragraph of a
       cell.
     */
    virtual void endOfCellAction();

    /**
       Action to be carried out at the end of the "table row"
       paragraph.
     */
    virtual void endOfRowAction();


public:
    TableManager();
    virtual ~TableManager(){}

    /**
       Set handler for resolveCurrentTable.

       @param pTableDataHandler     the handler
     */
    void setHandler(TableDataHandlerPointer_t pTableDataHandler);

    /**
       Set the current handle.

       @param rHandle     the handle
     */
    virtual void handle(const T & rHandle);

    /**
       Start a new table level.

       A new context is pushed onto the table data stack,
     */
    virtual void startLevel();

    /**
       End a table level.

       The current table is resolved and the context is popped from
       the stack.
     */
    virtual void endLevel();

    /**
       Handle the start of a paragraph group.
     */
    virtual void startParagraphGroup();

    /**
       Handle the end of a paragraph group.
    */
    virtual void endParagraphGroup();

    /**
       Handle an SPRM at curent handle.

       @param rSprm   the SPRM
     */
    virtual bool sprm(Sprm & rSprm);

    /**
       Handle properties at current handle.

       @param pProps   the properites
     */
    virtual void props(PropertiesPointer pProps);

    /**
       Handle occurrence of character 0x7.
     */
    virtual void handle0x7();

    /**
       Handle 8 bit text at current handle.

       @param data    array of characters
       @param len     number of characters to handle
     */
    virtual void text(const sal_uInt8 * data, size_t len);

    /**
       Handle 16 bit text at current handle.

       @param data    array of characters
       @param len     number of characters to handle
     */
    virtual void utext(const sal_uInt8 * data, size_t len);

    /**
       Handle properties of the current cell.

       @param pProps   the properties
     */
    virtual void cellProps(PropertiesPointer pProps);

    /**
       Handle properties of a certain cell in the current row.

       @paran i        index of the cell in the current row
       @param pProps   the properties
     */
    virtual void cellPropsByCell(unsigned int i, PropertiesPointer pProps);

    /**
       Handle properties of the current row.

       @param pProps   the properties
     */
    virtual void insertRowProps(PropertiesPointer pProps);

    /**
       Handle properties of the current table.

       @param pProps   the properties
     */
    virtual void insertTableProps(PropertiesPointer pProps);

    /**
       Return if table manager has detected paragraph to ignore.

       If this function returns true the current paragraph contains
       only control information, e.g. end of row.
     */
    virtual bool isIgnore() const;

    
#ifdef DEBUG_TABLE
    void setTagLogger(TagLogger::Pointer_t _tagLogger)
    {
        mpTableLogger = _tagLogger;
    }
#endif
};

template <typename T, typename PropertiesPointer>
TableManager<T, PropertiesPointer>::TableManager()
: mnTableDepthNew(0), mnTableDepth(0)
{
    setRowEnd(false);
    setInCell(false);
    setCellEnd(false);
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::cellDepth(sal_uInt32 nDepth)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
    {
        mpTableLogger->startElement("tablemanager.cellDepth");
        mpTableLogger->attribute("depth", nDepth);
        mpTableLogger->endElement("tablemanager.cellDepth");
    }
#endif

    mnTableDepthNew = nDepth;
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::inCell()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->element("tablemanager.inCell");
#endif
    setInCell(true);

    if (mnTableDepthNew < 1)
        mnTableDepthNew = 1;
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endCell()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->element("tablemanager.endCell");
#endif

    setCellEnd(true);
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endRow()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->element("tablemanager.endRow");
#endif

    setRowEnd(true);
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::setHandler
(typename TableDataHandler<T, PropertiesPointer>::Pointer_t pTableDataHandler)
{
    mpTableDataHandler = pTableDataHandler;
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::handle(const T & rHandle)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get())
    {
        mpTableLogger->startElement("tablemanager.handle");
        mpTableLogger->chars(toString(rHandle));
        mpTableLogger->endElement("tablemanager.handle");
    }
#endif

    setHandle(rHandle);
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::startLevel()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t pTableData;
        
        if (mTableDataStack.size() > 0)
            pTableData = mTableDataStack.top();
        
        mpTableLogger->startElement("tablemanager.startLevel");
        mpTableLogger->attribute("level", mTableDataStack.size());
        
        if (pTableData.get() != NULL)
            mpTableLogger->attribute("openCell", 
                                     pTableData->isCellOpen() ? "yes" : "no");
        
        mpTableLogger->endElement("tablemanager.startLevel");
    }
#endif

    typename TableData<T, PropertiesPointer>::Pointer_t pTableData
        (new TableData<T, PropertiesPointer>(mTableDataStack.size()));

    mTableDataStack.push(pTableData);
    mState.startLevel();
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endLevel()
{
    if (mpTableDataHandler.get() != NULL)
        resolveCurrentTable();

    mState.endLevel();
    mTableDataStack.pop();
    
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t pTableData;
        
        if (mTableDataStack.size() > 0)
            pTableData = mTableDataStack.top();

        mpTableLogger->startElement("tablemanager.endLevel");
        mpTableLogger->attribute("level", mTableDataStack.size());

        if (pTableData.get() != NULL)
            mpTableLogger->attribute("openCell", 
                                     pTableData->isCellOpen() ? "yes" : "no");
        
        mpTableLogger->endElement("tablemanager.endLevel");
    }
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::startParagraphGroup()
{
	mState.resetCellSpecifics();
    mnTableDepthNew = 0;
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endParagraphGroup()
{
    sal_Int32 nTableDepthDifference = mnTableDepthNew - mnTableDepth;

    PropertiesPointer pEmptyProps;
    
    while (nTableDepthDifference > 0)
    {
        ensureOpenCell(pEmptyProps);
        startLevel();

        --nTableDepthDifference;
    }
    while (nTableDepthDifference < 0)
    {
        endLevel();

        ++nTableDepthDifference;
    }

    mnTableDepth = mnTableDepthNew;

    if ( mnTableDepth > 0 )
    {
        typename TableData< T, PropertiesPointer >::Pointer_t pTableData = mTableDataStack.top();

        if ( isRowEnd() )
        {
            endOfRowAction();
            pTableData->endRow( getRowProps() );
            resetRowProps();
        }

        else if ( isInCell() )
        {
            ensureOpenCell( getCellProps() );

            if ( isCellEnd() )
            {
                endOfCellAction();
                closeCell( getHandle() );
            }
        }
        resetCellProps();
    }
}

template <typename T, typename PropertiesPointer>
bool TableManager<T, PropertiesPointer>::sprm(Sprm & rSprm)
{
    bool bRet = true;
    switch (rSprm.getId())
    {
    case NS_sprm::LN_PTableDepth:
        {
            Value::Pointer_t pValue = rSprm.getValue();

            cellDepth(pValue->getInt());
        }
        break;
    case NS_sprm::LN_PFInTable:
        inCell();
        break;
    case NS_sprm::LN_PCell:
        endCell();
        break;
    case NS_sprm::LN_PFTtp:
    case NS_sprm::LN_PRow:
        endRow();
        break;
    default:
        bRet = false;
    }
    return bRet;
}
template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::props(PropertiesPointer pProps)
{
    setProps(pProps);
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::handle0x7()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.handle0x7");
#endif

    if (mnTableDepthNew < 1)
        mnTableDepthNew = 1;

    if (isInCell())
        endCell();
    else
        endRow();

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.handle0x7");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::text(const sal_uInt8 * data, size_t len)
{
    // optimization: cell/row end characters are the last characters in a run
    if (len > 0)
    {
        if (data[len - 1] == 0x7)
            handle0x7();
    }
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::utext(const sal_uInt8 * data, size_t len)
{
    // optimization: cell/row end characters are the last characters in a run

    if (len > 0)
    {
        sal_Unicode nChar = data[(len - 1) * 2] + (data[(len - 1) * 2 + 1] << 8);
        if (nChar == 0x7)
            handle0x7();
    }
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::cellProps(PropertiesPointer pProps)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.cellProps");
#endif
    
    if(getCellProps().get())
        getCellProps()->insert( pProps );
    else
        setCellProps(pProps);

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.cellProps");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::cellPropsByCell
(unsigned int i, PropertiesPointer pProps)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.cellPropsByCell");
#endif

    mTableDataStack.top()->insertCellProperties(i, pProps);

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.cellPropsByCell");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::insertRowProps(PropertiesPointer pProps)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.insertRowProps");
#endif

    if( getRowProps().get() )
        getRowProps()->insert( pProps );
    else
        setRowProps(pProps);

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.insertRowProps");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::insertTableProps(PropertiesPointer pProps)
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.insertTableProps");
#endif

    if( getTableProps().get() )
        getTableProps()->insert( pProps );
    else
        setTableProps(pProps);

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.insertTableProps");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::resolveCurrentTable()
{
#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->startElement("tablemanager.resolveCurrentTable");
#endif

    if (mpTableDataHandler.get() != NULL)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t
            pTableData = mTableDataStack.top();

        const unsigned int nRows = pTableData->getRowCount();

        mpTableDataHandler->startTable(nRows, pTableData->getDepth(), getTableProps());

        for (unsigned int nRow = 0; nRow < nRows; ++nRow)
        {
            typename RowData<T, PropertiesPointer>::Pointer_t pRowData = pTableData->getRow(nRow);

            unsigned int nCells = pRowData->getCellCount();

            mpTableDataHandler->startRow(nCells, pRowData->getProperties());

            for (unsigned int nCell = 0; nCell < nCells; ++nCell)
            {
                mpTableDataHandler->startCell
                    (pRowData->getCellStart(nCell),
                     pRowData->getCellProperties(nCell));

                mpTableDataHandler->endCell(pRowData->getCellEnd(nCell));
            }

            mpTableDataHandler->endRow();
        }

        mpTableDataHandler->endTable( pTableData->getDepth() );
    }
    resetTableProps();

#ifdef DEBUG_TABLE
    if (mpTableLogger.get() != NULL)
        mpTableLogger->endElement("tablemanager.resolveCurrentTable");
#endif
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endOfCellAction()
{
}

template <typename T, typename PropertiesPointer>
void TableManager<T, PropertiesPointer>::endOfRowAction()
{
}

template <typename T, typename PropertiesPointer>
bool TableManager<T, PropertiesPointer>::isIgnore() const
{
    return isRowEnd();
}

template <typename T, typename PropertiesPointer>
void  TableManager<T, PropertiesPointer>::openCell
(const T & rHandle, PropertiesPointer pProps) 
{
#ifdef DEBUG_TABLE
    mpTableLogger->startElement("tablemanager.openCell");
    mpTableLogger->chars(toString(rHandle));
    mpTableLogger->endElement("tablemanager.openCell");
#endif

    if (mTableDataStack.size() > 0)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t
        pTableData = mTableDataStack.top();
        
        pTableData->addCell(rHandle, pProps);
    }
}

template <typename T, typename PropertiesPointer>
void  TableManager<T, PropertiesPointer>::closeCell
(const T & rHandle) 
{
#ifdef DEBUG_TABLE
    mpTableLogger->startElement("tablemanager.closeCell");
    mpTableLogger->chars(toString(rHandle));
    mpTableLogger->endElement("tablemanager.closeCell");
#endif
    
    if (mTableDataStack.size() > 0)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t 
        pTableData = mTableDataStack.top();
        
        pTableData->endCell(rHandle);
    }
}

template <typename T, typename PropertiesPointer>
void  TableManager<T, PropertiesPointer>::ensureOpenCell(PropertiesPointer pProps)
{
#ifdef DEBUG_TABLE
    mpTableLogger->startElement("tablemanager.ensureOpenCell");
#endif

    if (mTableDataStack.size() > 0)
    {
        typename TableData<T, PropertiesPointer>::Pointer_t 
        pTableData = mTableDataStack.top();
        
        if (pTableData.get() != NULL)
        {
            if (!pTableData->isCellOpen())
                openCell(getHandle(), pProps);
            else 
                pTableData->insertCellProperties(pProps);
        }
    }
#ifdef DEBUG_TABLE
    mpTableLogger->endElement("tablemanager.ensureOpenCell");
#endif
}
        
}

#endif // INCLUDED_TABLE_MANAGER_HXX
