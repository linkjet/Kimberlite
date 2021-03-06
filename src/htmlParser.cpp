#include "htmlParser.h"
W3Cinfo HtmlParser::htmlInfo = W3Cinfo();

HtmlParser::HtmlParser()  {
  QSqlQuery query;
  query.exec("SELECT NAME FROM THTML_TAG WHERE IFNEEDCLOSE = 0");
  while (query.next()) 
    htmlInfo.orphelinTags << query.value(0).toString();
  query.exec("SELECT NAME,IFNEEDNEWLINE FROM THTML_TAG WHERE IFNEEDNEWLINE <> 1");
  while (query.next()) {
    htmlInfo.noNewLineTags << query.value(0).toString();
    if ((query.value(1).toInt() >= 2) && (query.value(1).toInt() <= 3))
      ((query.value(1).toInt() == 2)?htmlInfo.needNewLineOnClose:htmlInfo.needNewLineOnOpen) << query.value(0).toString();
  }
}

QString HtmlParser::getTag(QString aTag) {
  QString tag;
  if (aTag.left(3) == "!--") 
    return "!--";
  tag = aTag.remove(0,(aTag[1] == '/')?2:1).trimmed(); //Remove < or </
  tag.chop((tag.indexOf(" ") == -1)?1:tag.size()-tag.indexOf(" ")); //Remove > and attribute //BUG fait with tag like <br/>
  return tag.toUpper();
}

void HtmlParser::split(QVector<QString> &tagList, QString &inputFile, uint index) {
  tagList.push_back(inputFile.left(index).trimmed());
  inputFile = inputFile.remove(0,index);
}

QVector<QString> HtmlParser::listTag(QString inputFile) {
  QVector<QString> tagList;
  while (inputFile != "") {
    while ((inputFile[0] == 0x20/*space*/) || (inputFile[0] == 0x09/*tab*/) || (inputFile[0] == 0x0A/*line break*/))
      inputFile = inputFile.remove(0,1);
    if (inputFile.indexOf("<") == -1) 
      split(tagList, inputFile, inputFile.size());
    else if (inputFile.indexOf("<") < inputFile.lastIndexOf("<",inputFile.indexOf(">"))) 
      split(tagList, inputFile, inputFile.lastIndexOf("<",inputFile.indexOf(">")-1));
    else if (inputFile.left(4) == "<!--") 
      split(tagList, inputFile, inputFile.indexOf("->")+2);
    else if ((inputFile.left(7).toUpper() == "<SCRIPT")) {
      split(tagList, inputFile, inputFile.indexOf(">")+1);
      if (inputFile.indexOf("</script>",Qt::CaseInsensitive) != 0) 
	split(tagList, inputFile, inputFile.indexOf("</script>",Qt::CaseInsensitive)-1);
    }
    else if (inputFile.indexOf("<") == 0) 
      split(tagList, inputFile, inputFile.indexOf(">")+1);
    else if (inputFile != "") 
      split(tagList, inputFile, inputFile.indexOf("<"));
  }
  return tagList;
}

QVector<uint> HtmlParser::levelParser(QVector<QString> tagList){
  QVector<uint> levelList;
  QString tag;
  for (int i=0; i < tagList.size();i++) {
    tag = getTag(tagList[i]);
    if (i==0)
      levelList.push_back(0);
    else if ((tagList[i][0] == '<') && (htmlInfo.orphelinTags.indexOf(tag) == -1))
      if (tagList[i][1] == '/')
        if ((tag == getTag(tagList[i-1])) && (tagList[i-1][1] != '/'))
          levelList.push_back(levelList[i-1]);
        else 
          levelList.push_back((levelList[i-1] > 0)?levelList[i-1]-1:0);
      else if ((tagList[i-1][0]  == '<') && ((tagList[i-1][1]  != '/')) && (htmlInfo.orphelinTags.indexOf(getTag(tagList[i-1])) == -1)) 
        levelList.push_back(levelList[i-1]+1);
      else 
        levelList.push_back(levelList[i-1]);
    else 
      if ((tagList[i-1][0]  == '<') && ((tagList[i-1][1]  != '/')) && (htmlInfo.orphelinTags.indexOf(getTag(tagList[i-1])) == -1)) 
        levelList.push_back(levelList[i-1]+1);
      else 
        levelList.push_back(levelList[i-1]);
  }
  return levelList;
}

HtmlData HtmlParser::getHtmlData(QString inputFile) {
  HtmlData pageData= {listTag(inputFile),levelParser(pageData.tagList)};
  return pageData;
}

QString HtmlParser::getParsedHtml(QString inputFile) {
  HtmlData pageData = getHtmlData(inputFile);
  return getParsedHtml(pageData);
}

QString HtmlParser::getParsedHtml(HtmlData &pageData) {
  QString tab, parsedHTML, tag2, previousTag;
  if (pageData.tagList.size() == 1)
    return pageData.tagList[0].trimmed();
  for (int j=0; j < pageData.tagList.size();j++) {
    tag2 = getTag(pageData.tagList[j]);
    tab.clear();
    for (uint k =0; k < pageData.levelList[j]; k++) 
      tab += "   ";
    if ((htmlInfo.noNewLineTags.indexOf(tag2) != -1) || (pageData.tagList[j][0] != '<')) {
      if ((pageData.tagList[j][0] == '<') && (htmlInfo.needNewLineOnOpen.indexOf(tag2) != -1) && (parsedHTML[(parsedHTML.size() !=0)?parsedHTML.size()-1:0] != '\n') && (pageData.tagList[j].left(2) != "</") && (parsedHTML.size() !=0))
        parsedHTML += "\n" + tab;
      if (parsedHTML[(parsedHTML.size())?parsedHTML.size()-1:0] == '\n')
        parsedHTML += tab + pageData.tagList[j];
      else 
        parsedHTML += pageData.tagList[j];
      if ((pageData.tagList[j].left(2) == "</") && (htmlInfo.needNewLineOnClose.indexOf(tag2) != -1)) 
        parsedHTML += "\n";
    }
    else {
      if ((parsedHTML.right(1) != "\n") && (parsedHTML != "")) 
        parsedHTML += "\n";
      parsedHTML += tab + pageData.tagList[j].trimmed() + "\n";
    }
  }
  return parsedHTML;
}

void HtmlParser::setAttribute(HtmlData &pageData, QString tag, uint index, QString attribute, QString value) {
  uint i=0;
  for (uint j =0; j < pageData.tagList.size(); j++)
    if ((HtmlParser::getTag(pageData.tagList[j]).toUpper() == tag.toUpper()) && (pageData.tagList[j][1] != '/'))
      if (i == index) {
	pageData.tagList[j] = setAttribute(pageData.tagList[j],attribute,value);
	break;
      }
      else
	i++;
}

QString HtmlParser::setAttribute(QString tag, QString attribute, QString value) {
  int start,length,isSet;
  getAttribute(tag,attribute,&start,&length,&isSet);
  (!isSet)?tag.insert(tag.count() - 1, " " + attribute + "=\"" + value + "\""):tag.replace(start,length,value);
  if (value.isEmpty())
    tag.remove(tag.toUpper().indexOf(attribute.toUpper()) -1,attribute.size()+4);
  return tag;
}

QString HtmlParser::getAttribute(QString tag, QString attribute, int* start, int* length, int* isSet) {
  int position(tag.toLower().indexOf(attribute.toLower()));
  if (start == (int*)UNSET)
    start = new int(), length = new int(), isSet = new int;
  isSet = false;
  if (tag[position+attribute.count()] == '=') {
    (*isSet) = (*start) = tag.indexOf("=",position)+2;
    (*length) = tag.indexOf((tag.indexOf(" ",position) != -1)?" ":">",position)-2 - (position+1+attribute.count());
    return (position == -1)?NULL:tag.mid((*start),(*length));
  }
  else
    return NULL; //Use isSet pointer to check if it exist, don't rely on NULL
}

HtmlData HtmlParser::getElementsByAttribute(HtmlData pageData, QString attribute, QString value, uint occurence) {
  int curOccurence(0),levelCount(0),depth(0),isIn(false);
  HtmlData toReturn;
  QString tagName;
  for (int i =0; i < pageData.tagList.size(); i++) {
    if (isIn) {
      toReturn.append(pageData.tagList[i], pageData.levelList[i]);
      if (HtmlParser::getTag(pageData.tagList[i]) == HtmlParser::getTag(tagName))
        depth += 1*(pageData.tagList[i].left(2) == "</")?-1:1;
      if ((!depth) || (htmlInfo.orphelinTags.indexOf(HtmlParser::getTag(tagName)) != -1))
        break;
    }
    else if (HtmlParser::getAttribute(tagName = pageData.tagList[i], attribute) == value )
      ((curOccurence == occurence)?isIn:curOccurence)++;
  }
  return toReturn;
}