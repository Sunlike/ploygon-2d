#include "StdAfx.h"
#include "RectangleContainer.h"
#include <algorithm>


// �ȽϷ���
struct CPolygonCompar:binary_function<CExentedPolygon*,CExentedPolygon*,bool>
{
	bool operator()(CExentedPolygon*p1,CExentedPolygon*p2)const
	{
		if(p1!=NULL && p1!= NULL)
		{
			return p1->equal(p2);
		}
		return false;
	}
};


CRectangleContainer::CRectangleContainer(int x,int y,int width, int height)
:m_container(NULL)
,m_blankArea(width*height)
,m_area(width*height)
,m_listSize(0)
,m_x(x)
,m_y(y)
,m_width(width)
,m_height(height)
{
	AcGePoint3dArray points;
	points.append(AcGePoint3d(x,y,0));
	points.append(AcGePoint3d(x,y+height,0));
	points.append(AcGePoint3d(x+width,y+height,0));
	points.append(AcGePoint3d(x+width,y,0));
	m_container = new CExentedPolygon(points);
	for(int i = 0; i < 5;++i)
	{
		m_polygons_inside[i] = vector<CExentedPolygon*>();
	}  
}

CRectangleContainer::~CRectangleContainer(void)
{
}

vector<CExentedPolygon*> CRectangleContainer::getAllPolygons()
{

	vector<CExentedPolygon*> resultList;

	for(int i =0;i < 5;++i)
	{
		vector<CExentedPolygon*> tmpList = m_polygons_inside[i];
		std::copy(tmpList.begin(), tmpList.end(), std::back_inserter(resultList));
	}
	return resultList;
}
int CRectangleContainer::getQuadrant(CExentedPolygon* polygon)
{
	return calculateQuadrant(polygon,m_x,m_y,m_width,m_height);
}
int CRectangleContainer::calculateQuadrant(CExentedPolygon* polygon, int x, int y, int width, int height)
{

	AcGePoint3d center;
	center.x = x+width/2;
	center.y = y+height/2;

	CRectangleContainer* section1 = new CRectangleContainer(x,y,width/2,height/2);
	CRectangleContainer* section2 = new CRectangleContainer(center.x,y,width/2,height/2);
	CRectangleContainer* section3 = new CRectangleContainer(x,center.y,width/2,height/2);
	CRectangleContainer* section4 = new CRectangleContainer(center.x,center.y,width/2,height/2);

	if(section1->contains(polygon)) 
	{
		return 1;
	} 
	else if(section2->contains(polygon)) 
	{
		return 2;
	} 
	else if(section3->contains(polygon)) 
	{
		return 3;
	} 
	else if(section4->contains(polygon)) 
	{
		return 4;
	} 
 
	return 0;

}
bool CRectangleContainer::contains(CExentedPolygon*polygon)
{
	AcGePoint3dArray points = polygon->getVerts();
	int size = points.length();
	for(int i = 0; i < size; ++i)
	{
		if(!contains(points.at(i).x,points.at(i).y))
		{
			return false;
		}
	}

	return true;
}
bool  CRectangleContainer::contains(double x,double y)
{
	if((x >= m_x) && (x <= m_x+m_width) && (y >= m_y) && (y <= m_y+m_height))
	{
		return true;
	}
	return false;
}

bool CRectangleContainer::remove(CExentedPolygon* polygon)
{
	vector<CExentedPolygon*>& polyList = this->m_polygons_inside[polygon->getQuadrant()];
	
	vector<CExentedPolygon*>::iterator iter = find_if(polyList.begin(),polyList.end(),bind2nd(CPolygonCompar(),polygon));
	
	bool res = polyList.erase(iter) != polyList.end();
	if(res)
	{
		double area= 0;
		polygon->getArea(area);
		this->m_blankArea += area;
		this->m_listSize--;
	}
	return res;
}

void CRectangleContainer::put(CExentedPolygon* polygon)
{
	(this->m_polygons_inside[polygon->getQuadrant()]).push_back(polygon);
	double area = 0.0;
	polygon->getArea(area);
	this->m_blankArea-=area;
	m_listSize = 0;
	for(int i = 0; i < 5; ++i)
	{
		m_listSize+=(this->m_polygons_inside[i]).size();
	}

	int size = polygon->getVerts().length();
	acutPrintf(_T("%d: %d-edges  %.2f%%-coverage  %.2f-area\n\r"),m_listSize,size,getCoverageRatio()*100,area);
}
bool CRectangleContainer::safePut(CExentedPolygon* polygon)
{
	if(!this->contains(polygon))
	{
		return false;
	}

	int section = getQuadrant(polygon);
	polygon->setQuadrant(section);

	{	
		vector<CExentedPolygon*>& polyList = this->m_polygons_inside[0];
		for(int i = 0;i < polyList.size();++i)
		{
			if(polygon->intersects(polyList[i]))
			{
				return false;
			}
		}
	}

	if(section == 0)
	{
		for(int i = 1; i < 5;++i)
		{
			vector<CExentedPolygon*>& polyList = this->m_polygons_inside[i];
			for(int j = 0;j < polyList.size();++j)
			{
				if(polygon->intersects(polyList[j]))
				{
					return false;
				}
			}
		}
	}
	else
	{
		vector<CExentedPolygon*>& polyList = this->m_polygons_inside[section];
		for(int j = 0;j < polyList.size();++j)
		{
			if(polygon->intersects(polyList[j]))
			{
				return false;
			}
		}
	}

	this->put(polygon);
	return true;

}