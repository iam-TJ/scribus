/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "cmdannotations.h"
#include "cmdutil.h"
#include "util.h"
#include "scribuscore.h"

static PyObject *getLinkData(PyObject *rv, int page, const QString& action);
static void prepareannotation(PageItem *i);
static void setactioncoords(Annotation &a, int x, int y);
static bool testPageItem(PageItem *i);
static void add_text_to_dict(PyObject *drv, PageItem *i);

PyObject *scribus_isannotated(PyObject * /*self*/, PyObject* args, PyObject *keywds)
{
	char *name = const_cast<char*>("");  
	PyObject *deannotate = Py_False;
	char *kwlist[] = {const_cast<char*>(""),const_cast<char*>("deannotate"), nullptr};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "|esO", kwlist, "utf-8", &name, &deannotate))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	PageItem *i = GetUniqueItem(QString::fromUtf8(name));
	if (i == nullptr)
		return nullptr;

	if (i->isAnnotation())
	{
		if (PyObject_IsTrue(deannotate)==1)
		{
			i->setIsAnnotation(false);
			Py_RETURN_NONE;
		}

		Annotation a = i->annotation();
		int atype = a.Type();
		int actype = a.ActionType();

		PyObject *drv = PyDict_New();
		if (atype == Annotation::Link && (actype == Annotation::Action_GoToR_FileAbs || actype == Annotation::Action_GoToR_FileRel))
		{
			char *name3;
			if (actype== Annotation::Action_GoToR_FileAbs) 
				name3 =const_cast<char*>("Link File Absolute");
			else
				name3 =const_cast<char*>("Link File Relative");

			getLinkData(drv, a.Ziel(), a.Action());
			const char path[] = "path";
			PyObject *pathkey = PyString_FromString(path);
			PyObject *pathvalue = PyString_FromString(a.Extern().toUtf8());
			PyDict_SetItem(drv, pathkey, pathvalue);
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name3, drv);
			return rv;
		}
		else if (atype == Annotation::Link && actype == Annotation::Action_URI)
		{
			const char uri[] = "uri";
			PyObject *ukey = PyString_FromString(uri);
			PyObject *uval = PyString_FromString(a.Extern().toUtf8());
			PyDict_SetItem(drv, ukey, uval);
			add_text_to_dict(drv, i);
			char *name4= const_cast<char*>("Link URI");
			PyObject *rv = Py_BuildValue("(sO)", name4, drv);
			return rv;
		}
		else if (atype == Annotation::Link)
		{
			getLinkData(drv, a.Ziel(), a.Action());
			const char name2[] = "Link";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name2, drv);
			return rv;
		}
		else if (atype == Annotation::Button)
		{
			const char name5[] = "Button";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name5, drv);
			return rv;
		}
		else if (atype == Annotation::RadioButton)
		{
			const char name4[] = "RadioButton";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name4, drv);
			return rv;
		}
		else if (atype == Annotation::Textfield)
		{
			const char name6[] = "Textfield";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name6, drv);
			return rv;
		}
		else if (atype == Annotation::Checkbox)
		{
			const char name7[] = "Checkbox";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name7, drv);
			return rv;
		}
		else if (atype == Annotation::Combobox)
		{
			const char name4[] = "Combobox";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name4, drv);
			return rv;
		}
		else if (atype == Annotation::Listbox)
		{
			const char name8[] = "Listbox";
			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name8, drv);
			return rv;
		}
		else if (atype == Annotation::Text)
		{
			/** icons: 0 "Note", 1 "Comment", 2 "Key",
			3 "Help", 4 "NewParagraph", 5 "Paragraph", 6 "Insert",
			7 "Cross", 8 "Circle"
			*/ 
			const char name9[] = "Text";
			int icon = a.Icon();
			const char *icons[] = {"Note","Comment", 
			  "Key", "Help", 
			  "NewParagraph","Paragraph",
			  "Insert","Cross",
			  "Circle", nullptr
			};
			if (icon >= 0 && icon < 9)
			{
				PyObject *iconkey = PyString_FromString("icon");
				PyObject *iconvalue = PyString_FromString(icons[icon]);
				PyDict_SetItem(drv, iconkey, iconvalue);
			}

			PyObject *openkey = PyString_FromString("open");
			PyObject *open = Py_False;
			if (a.IsAnOpen())
				open = Py_True;
			PyDict_SetItem(drv, openkey, open);

			add_text_to_dict(drv, i);
			PyObject *rv = Py_BuildValue("(sO)", name9, drv);
			return rv;
		}
		else if (atype == Annotation::Annot3D)
		{
			const char a3dname[] = "Annot3D";
			PyObject *rv = Py_BuildValue("(sO)",a3dname, drv);
			return rv;
		}
		else
		{
			const char unknown[] = "Unknown Annotation";
			PyObject *rv = Py_BuildValue("(sO)", unknown, drv);
			return rv;
		}
	}

	Py_RETURN_NONE;
}


PyObject *scribus_setlinkannotation(PyObject* /* self */, PyObject* args)
{
	char *name = const_cast<char*>("");
	int page, x, y;

	if (!PyArg_ParseTuple(args, "iii|es", &page, &x, &y, "utf-8", &name))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	PageItem *i = GetUniqueItem(QString::fromUtf8(name));
	if (!testPageItem(i))
		return nullptr;

	int numpages = ScCore->primaryMainWindow()->doc->Pages->count();
	if (page <= 0 || page > numpages){
		QString qnum = QString("%1").arg(numpages);
		PyErr_SetString(PyExc_RuntimeError,
			QObject::tr("which must be 1 to " + qnum.toUtf8(), "python error").toLocal8Bit().constData());
		return nullptr;
	}

	prepareannotation(i);
	Annotation &a = i->annotation();
	a.setType(Annotation::Link);
	page -= 1;
	a.setZiel(page);
	setactioncoords(a, x, y);
	a.setExtern(QString::fromUtf8(""));
	a.setActionType(Annotation::Action_GoTo);

//	Py_INCREF(Py_None);
//	return Py_None;
	Py_RETURN_NONE;
}


PyObject *scribus_setfileannotation(PyObject * /*self*/, PyObject* args, PyObject *keywds)
{
	char *path;
	int page, x, y;
	char *name = const_cast<char*>("");
	PyObject *absolute = Py_True;

	char *kwlist[] = {const_cast<char*>("path"), const_cast<char*>("page"), 
			  const_cast<char*>("x"), const_cast<char*>("y"), 
			  const_cast<char*>("name"),const_cast<char*>("absolute"), nullptr};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "esiii|esO", kwlist,
					 "utf-8", &path, &page, &x, &y,
					 "utf-8", &name, &absolute))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	PageItem *i = GetUniqueItem(QString::fromUtf8(name));
	if (!testPageItem(i))
		return nullptr;

	prepareannotation(i);
	Annotation &a = i->annotation();
	a.setType(Annotation::Link);
	a.setZiel(page - 1);
	a.setExtern(QString::fromUtf8(path));
	setactioncoords(a, x, y);

	if (PyObject_IsTrue(absolute)==1)
		a.setActionType(Annotation::Action_GoToR_FileAbs);
	else
		a.setActionType(Annotation::Action_GoToR_FileRel);

	Py_RETURN_NONE;

}

PyObject *scribus_seturiannotation(PyObject * /*self*/, PyObject* args)
{
	char *uri;
	char *name = const_cast<char*>("");

	if (!PyArg_ParseTuple(args, "es|es","utf-8" ,&uri,"utf-8", &name))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	PageItem *i = GetUniqueItem(QString::fromUtf8(name));
	if (!testPageItem(i))
		return nullptr;
		
	prepareannotation(i);
	Annotation &a = i->annotation();
	a.setAction(QString::fromUtf8(""));
	a.setExtern(QString::fromUtf8(uri));
	a.setActionType(Annotation::Action_URI);
	a.setType(Annotation::Link);

	Py_RETURN_NONE;
}


PyObject *scribus_settextannotation(PyObject * /*self*/, PyObject* args)
{
	/** icons: 0 "Note", 1 "Comment", 2 "Key",
		   3 "Help", 4 "NewParagraph", 5 "Paragraph",
		   6 "Insert", 7 "Cross", 8 "Circle"
	*/ 
	int icon;
	PyObject *isopen = Py_False;
	char *name = const_cast<char*>("");
	/** icons: 0 "Note", 1 "Comment", 2 "Key",
			3 "Help", 4 "NewParagraph", 5 "Paragraph", 6 "Insert",
			7 "Cross", 8 "Circle"

	*/ 

	if (!PyArg_ParseTuple(args, "iO|es",&icon,&isopen,"utf-8", &name))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	if (icon < 0 || icon > 8)
	{
		PyErr_SetString(PyExc_RuntimeError,
			QObject::tr("Icon must be 0 to 8", "python error").toLocal8Bit().constData());
		return nullptr;
	}

	PageItem *i = GetUniqueItem(QString::fromUtf8(name));
	if (!testPageItem(i))
		return nullptr;
	
	prepareannotation(i);

	Annotation &a = i->annotation();
	a.setAnOpen(PyObject_IsTrue(isopen));
	a.setActionType(Annotation::Action_None);
	a.setIcon(icon);
	a.setExtern(QString::fromUtf8(""));
	a.setAction(QString::fromUtf8(""));
	a.setType(Annotation::Text);

	Py_RETURN_NONE;
}



PyObject *scribus_createpdfannotation(PyObject * /*self*/, PyObject* args)
{
	enum{PDFBUTTON, PDFRADIOBUTTON, PDFTEXTFIELD, PDFCHECKBOX, PDFCOMBOBOX, PDFLISTBOX,
	     PDFTEXTANNOTATION, PDFLINKANNOTATION, PDF3DANNOTATION};

	int which;
	double x, y, w, h;
	char *name = const_cast<char*>("");

	if (!PyArg_ParseTuple(args, "idddd|es", &which, &x, &y, &w, &h, "utf-8", &name))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	if (which < 0 || which > 8){
		PyErr_SetString(PyExc_RuntimeError,
			QObject::tr("which must be 0 to 8", "python error").toLocal8Bit().constData());
		return nullptr;
	}

	ScribusDoc *m_doc = ScCore->primaryMainWindow()->doc;

	int i;
	if (which < 8)
	{
		i = m_doc->itemAdd(PageItem::TextFrame, 
		                   PageItem::Unspecified,
		                   pageUnitXToDocX(x),
		                   pageUnitYToDocY(y),
		                   ValueToPoint(w),
		                   ValueToPoint(h),
		                   m_doc->itemToolPrefs().shapeLineWidth, 
		                   CommonStrings::None,
		                   m_doc->itemToolPrefs().textColor);
	}
	else
	{
		bool hasosg=false;
	#ifdef HAVE_OSG
		hasosg=true;
	#endif
		if (hasosg)
		{
			i = m_doc->itemAdd(PageItem::OSGFrame, 
			                   PageItem::Unspecified, 
			                   pageUnitXToDocX(x),
			                   pageUnitYToDocY(y),
			                   ValueToPoint(w),
			                   ValueToPoint(h),
			                   m_doc->itemToolPrefs().shapeLineWidth, 
			                   m_doc->itemToolPrefs().imageFillColor, 
			                   m_doc->itemToolPrefs().imageStrokeColor);
		}
		else{
			PyErr_SetString(PyExc_RuntimeError,
			QObject::tr("Doesn't have OSG can't create 3DAnnotation", "python error").toLocal8Bit().constData());
			return nullptr;
		}
	}


	PageItem *pi = m_doc->Items->at(i);
	pi->AutoName=false;



	if (strlen(name) > 0)
	{
		QString objName = QString::fromUtf8(name);
		if (!ItemExists(objName))
			m_doc->Items->at(i)->setItemName(objName);
	}
	else
	{
		QString inames[] = {
			CommonStrings::itemName_PushButton,
			CommonStrings::itemName_RadioButton,
			CommonStrings::itemName_TextField,
			CommonStrings::itemName_CheckBox,
			CommonStrings::itemName_ComboBox,
			CommonStrings::itemName_ListBox,
			CommonStrings::itemName_TextAnnotation,
			CommonStrings::itemName_LinkAnnotation,
			QObject::tr("3DAnnot")
		};
		QString iname = inames[which] + QString("%1").arg(m_doc->TotalItems);
		pi->setItemName(iname);
	}


	pi->setIsAnnotation(true);
	Annotation &a = pi->annotation();

	Annotation::AnnotationType atypes[] = {
		Annotation::Button,    Annotation::RadioButton,
		Annotation::Textfield, Annotation::Checkbox,
		Annotation::Checkbox,  Annotation::Combobox,
		Annotation::Listbox,   Annotation::Text,
		Annotation::Link,      Annotation::Annot3D
	};
	a.setType(atypes[which]);

	switch(which)
	{
		case PDFBUTTON:
			a.setFlag(Annotation::Flag_PushButton);
			break;
		case PDFRADIOBUTTON:
			a.setFlag(Annotation::Flag_Radio | Annotation::Flag_NoToggleToOff);
			break;
		case PDFCOMBOBOX:
			a.setFlag(Annotation::Flag_Combo);
			break;
		case PDFLINKANNOTATION:
			a.setZiel(m_doc->currentPage()->pageNr());
			a.setAction("0 0");
			a.setActionType(Annotation::Action_GoTo);
			pi->setTextFlowMode(PageItem::TextFlowDisabled);
			break;
	}
	
	return PyString_FromString(m_doc->Items->at(i)->itemName().toUtf8());
}


/*! HACK: this removes "warning: 'blah' defined but not used" compiler warnings
with header files structure untouched (docstrings are kept near declarations)
PV */
void cmdannotationsdocwarnings() 
{
    QStringList s;
    s << scribus_setlinkannotation__doc__
      << scribus_isannotated__doc__
      << scribus_setfileannotation__doc__
      << scribus_seturiannotation__doc__
      << scribus_settextannotation__doc__
      << scribus_createpdfannotation__doc__;
}


//HELPER FUNCTIONS

PyObject *getLinkData(PyObject *rv,int page, const QString& action)
{
	int x, y;

	const char pagenum[] = "page";
	PyObject *pagekey = PyString_FromString(pagenum);
	PyObject *pagevalue = PyInt_FromLong((long)page);
	PyDict_SetItem(rv, pagekey, pagevalue);
	
	QStringList qsl = action.split(" ", QString::SkipEmptyParts);

	x = qsl[0].toInt();
	const char x2[] = "x";
	PyObject *xkey = PyString_FromString(x2);
	PyObject *xvalue = PyInt_FromLong((long)x);
	PyDict_SetItem(rv, xkey, xvalue);

	int height =ScCore->primaryMainWindow()->doc->pageHeight();
	y = height - qsl[1].toInt();
	const char y2[] = "y";
	PyObject *ykey = PyString_FromString(y2);
	PyObject *yvalue = PyInt_FromLong((long)y);
	PyDict_SetItem(rv, ykey, yvalue);

	return rv;

}

static void prepareannotation(PageItem *i)
{
	if (i->isBookmark)
	{
	  i->isBookmark = false;
	  ScCore->primaryMainWindow()->DelBookMark(i);
	}
	i->setIsAnnotation(true);
}

static void setactioncoords(Annotation &a, int x, int y)
{
	QString xstring, ystring;
	int height =ScCore->primaryMainWindow()->doc->pageHeight();
	a.setAction(xstring.setNum(x) + " " + ystring.setNum(height - y) + " 0");
}

static bool testPageItem(PageItem *i)
{
	if (i == nullptr)
		return false;
	if (!i->asTextFrame())
	{
		PyErr_SetString(WrongFrameTypeError, 
				QObject::tr("Can't set annotation on a non-text frame", "python error").toLocal8Bit().constData());
		return false;
	}

	return true;
}

static void add_text_to_dict(PyObject *drv, PageItem * i)
{
	const char text[] = "text";
	PyObject *textkey = PyString_FromString(text);
	QString txt = i->itemText.text(0, i->itemText.length());
	PyObject *textvalue = PyString_FromString(txt.toUtf8());
	PyDict_SetItem(drv, textkey, textvalue);

	Annotation &a = i->annotation();
	int actype = a.ActionType();

	if (actype == Annotation::Action_JavaScript)
	{
		const char text[] = "javascript";
		PyObject *jskey = PyString_FromString(text);
		PyObject *jsvalue = PyString_FromString(i->annotation().Action().toUtf8());
		PyDict_SetItem(drv, jskey, jsvalue);
	}

	const char *aactions[] = {  "None",  "JavaScript", 
			            "Goto",  "Submit Form", 
			            "Reset Form",  "Import Data", 
			            "Unknown",  "Goto File Relative", 
			            "URI",  "Goto File Relative" , 
						"Named", nullptr };

	const char action[] = "action";
	PyObject *akey = PyString_FromString(action);
	if (actype > 10)
		actype = 6;
	PyObject *avalue = PyString_FromString(aactions[actype]);
	PyDict_SetItem(drv, akey, avalue);

	int atype = a.Type();
	if (atype == Annotation::Checkbox || atype == Annotation::RadioButton)
	{
		const char checked[] = "checked";
		PyObject *checkkey = PyString_FromString(checked);
		PyObject *checkvalue = Py_False;
		if (a.IsChk())
			checkvalue = Py_True;
		PyDict_SetItem(drv, checkkey, checkvalue);
	}

	if (atype == Annotation::Combobox || atype == Annotation::Listbox)
	{
		const char editable[] = "editable";
		PyObject *ekey = PyString_FromString(editable);

		PyObject *edit = Py_False;
		int result = Annotation::Flag_Edit & a.Flag();
		if (result == Annotation::Flag_Edit) 
			edit = Py_True;
		PyDict_SetItem(drv, ekey, edit);
	}
}


