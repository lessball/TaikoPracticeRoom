#include <Game2DAnimation/Animation2DManager.h>
#include <Game2DAnimation/Animation2D.h>
#include <FileResource/XmlFileReader.h>
#include <LogPrint.h>
#include <Matrix2DMath.h>
#include <boost/algorithm/string.hpp>
#include <string.h>
#include <stdio.h>
#include <assert.h>
using namespace rapidxml;
using namespace std;

Animation2DManager Animation2DManager::s_singleton;

Animation2DResource::Animation2DResource(const char *name)
{
	m_name = name;
	m_frameInfo = NULL;
	m_numFrameInfo = 0;
}
Animation2DResource::~Animation2DResource()
{
	assert(m_actionMap.size() == 0);
}
//用于解析xml的结构
struct _image
{
	string name;
	TextureResource *tex;
};
struct _module
{
	float clip[4];
};
struct _frame
{
	xml_node<> *xnode;
	Game2DImageNode *imageNode;
	int info;
};
bool Animation2DResource::load(const char *name, bool delay)
{
	XmlFileReader xfile(name);
	xml_node<> *xroot = xfile.getRootNode();
	if(xroot == NULL)
	{
		LOG_PRINT("failed to load animation2D : %s\n", name);
		return false;
	}
	map<int, _image> imageMap;
	map<int, _module> moduleMap; // (modulesId<<16)|moduleId 作为key。要求id不超过16位
	for(xml_node<> *xmodules = xroot->first_node("modules"); xmodules != NULL; xmodules = xmodules->next_sibling("modules"))
	{
		xml_attribute<> *xid = xmodules->first_attribute("name");
		xml_attribute<> *ximage = xmodules->first_attribute("image");
		int id;
		if(xid == NULL || ximage == NULL || sscanf(xid->value(), "%d", &id) < 1)
			continue;
		const char *imageName = ximage->value();
		if(strrchr(imageName, '/') != NULL)
		{
			_image img = {imageName, NULL};
			imageMap[id] = img;
		}else
		{
			const char *pPathEnd = strrchr(name, '/');
			if(pPathEnd != NULL)
			{
				string imagePath(name, pPathEnd-name+1);
				imagePath += imageName;
				_image img = {imagePath, NULL};
				imageMap[id] = img;
			}else
			{
				_image img = {imageName, NULL};
				imageMap[id] = img;
			}
		}
		for(xml_node<> *xmodule = xmodules->first_node("module"); xmodule != NULL; xmodule = xmodule->next_sibling("module"))
		{
			xml_attribute<> *xmid = xmodule->first_attribute("name");
			xml_attribute<> *xclip = xmodule->first_attribute("clip");
			int mid;
			_module mod;
			if(xmid == NULL || xclip == NULL || sscanf(xmid->value(), "%d", &mid) < 1 || sscanf(xclip->value(), "%g%*c%g%*c%g%*c%g", mod.clip, mod.clip+1, mod.clip+2, mod.clip+3) < 4)
				continue;
			assert(id < 0x10000 && mid < 0x10000);
			moduleMap[(id<<16) | mid] = mod;
		}
	}
	map<int, _frame> frameMap;
	xml_node<> *xframes = xroot->first_node("frames");
	if(xframes != NULL)
	{
		for(xml_node<> *xframe = xframes->first_node("frame"); xframe != NULL; xframe = xframe->next_sibling("frame"))
		{
			xml_attribute<> *xid = xframe->first_attribute("name");
			int id;
			if(xid == NULL || sscanf(xid->value(), "%d", &id) < 1)
				continue;
			_frame &tframe = frameMap[id];
			tframe.xnode = xframe;
			tframe.imageNode = NULL;
			tframe.info = -1;
		}
	}
	vector<Animation2DFrameInfo> frameInfo;
	xml_node<> *xactions = xroot->first_node("actions");
	if(xactions != NULL)
	{
		for(xml_node<> *xaction = xactions->first_node("action"); xaction != NULL; xaction = xaction->next_sibling("action"))
		{
			xml_attribute<> *xname = xaction->first_attribute("name");
			if(xname == NULL)
				continue;
			if(m_defaultAction.empty())
				m_defaultAction = xname->value();
			Animation2DAction *paction = &(m_actionMap[xname->value()]);
			paction->loop = true;
			xml_attribute<> *xloop = xaction->first_attribute("loop");
			if(xloop != NULL)
			{
				int iloop = 1;
				if(boost::iequals(xloop->value(), "false") || (sscanf(xloop->value(), "%d", &iloop)==1 && iloop==0))
					paction->loop = false;
			}
			paction->node = new Game2DImageNode();
			xml_attribute<> *xadd = xaction->first_attribute("addMix");
			bool addblend = false;
			if(xadd != NULL)
			{
				int iadd = 0;
				if(boost::iequals(xadd->value(), "true") || (sscanf(xadd->value(), "%d", &iadd)==1 && iadd>0))
					addblend = true;
			}
			vector<pair<float, int> > framedata;
			for(xml_node<> *xseq = xaction->first_node("sequence"); xseq != NULL; xseq = xseq->next_sibling("sequence"))
			{
				xml_attribute<> *xfid = xseq->first_attribute("frame");
				xml_attribute<> *xtime = xseq->first_attribute("duration");
				int fid;
				float time;
				if(xfid == NULL || xtime == NULL || sscanf(xfid->value(), "%d", &fid) < 1 || sscanf(xtime->value(), "%g", &time) < 1)
					continue;
				map<int, _frame>::iterator iframe = frameMap.find(fid);
				if(iframe == frameMap.end())
					continue;
				_frame &frame = iframe->second;
				if(frame.imageNode == NULL)
				{
					Animation2DFrameInfo finfo;
					xml_node<> *xcols = frame.xnode->first_node("collisions");
					if(xcols != NULL)
					{
						for(xml_node<> *xcol = xcols->first_node("collision"); xcol != NULL; xcol = xcol->next_sibling("collision"))
						{
							xml_attribute<> *xrect = xcol->first_attribute("rect");
							float x, y, w, h;
							if(xrect != NULL && sscanf(xrect->value(), "%g%*c%g%*c%g%*c%g", &x, &y, &w, &h) == 4)
							{
								Animation2DFrameInfo::Rect rec = {x, x+w, y, y+h};
								finfo.collisionRect.push_back(rec);
							}
						}
					}
					xml_node<> *xref = frame.xnode->first_node("reference");
					if(xref != NULL)
					{
						for(xml_node<> *xpoint = xref->first_node("point"); xpoint != NULL; xpoint = xpoint->next_sibling("point"))
						{
							xml_attribute<> *xpos = xpoint->first_attribute("pos");
							Animation2DFrameInfo::Point pt;
							if(xpos != NULL && sscanf(xpos->value(), "%g%*c%g", &(pt.x), &(pt.y)) == 2)
								finfo.referencePoint.push_back(pt);
						}
					}
					if(finfo.collisionRect.size() > 0 || finfo.referencePoint.size() > 0)
					{
						frame.info = (int)frameInfo.size();
						frameInfo.push_back(Animation2DFrameInfo());
						frameInfo[frame.info].collisionRect.swap(finfo.collisionRect);
						frameInfo[frame.info].referencePoint.swap(finfo.referencePoint);
					}
					for(xml_node<> *xsprite = frame.xnode->first_node("sprite"); xsprite != NULL; xsprite = xsprite->next_sibling("sprite"))
					{
						xml_attribute<> *xmodsid = xsprite->first_attribute("modules");
						xml_attribute<> *xmodid = xsprite->first_attribute("module");
						int modsid, modid;
						if(xmodsid == NULL || xmodid == NULL || sscanf(xmodsid->value(), "%d", &modsid) < 1 || sscanf(xmodid->value(), "%d", &modid) < 1)
							continue;
						map<int, _image>::iterator iimage = imageMap.find(modsid);
						assert(modsid < 0x10000 && modid < 0x10000);
						map<int, _module>::iterator imod = moduleMap.find((modsid<<16) | modid);
						if(iimage == imageMap.end() || imod == moduleMap.end())
							continue;
						float pos[2];
						xml_attribute<> *xpos = xsprite->first_attribute("pos");
						if(xpos == NULL || sscanf(xpos->value(), "%g%*c%g", pos, pos+1) < 2)
							pos[0] = pos[1] = 0.0f;
						float size[2];
						xml_attribute<> *xsize = xsprite->first_attribute("size");
						if(xsize == NULL || sscanf(xsize->value(), "%g%*c%g", size, size+1) < 2)
							size[0] = size[1] = 1.0f;
						float rot;
						xml_attribute<> *xrot = xsprite->first_attribute("degrees");
						if(xrot == NULL || sscanf(xrot->value(), "%g", &rot) < 1)
							rot = 0.0f;
						float pivot[2];
						xml_attribute<> *xpivot = xsprite->first_attribute("pivot");
						if(xpivot == NULL || sscanf(xpivot->value(), "%g%*c%g", pivot, pivot+1) < 2)
							pivot[0] = pivot[1] = 0.0f;
						int color;
						xml_attribute<> *xcolor = xsprite->first_attribute("color");
						if(xcolor == NULL || sscanf(xcolor->value(), "%x", &color) < 1)
							color = 0xffffffff;
						Game2DImageNode *node = new Game2DImageNode();
						node->setDrawMode(Game2DImageNode::DRAW_MODE_TEXTURE);
						node->setDrawFlag(0);
						if(iimage->second.tex == NULL)
							iimage->second.tex = TextureManager::getSingleton()->get(iimage->second.name.c_str(), delay);
						node->setTextureResource(iimage->second.tex);
						node->setTexcoordRange(imod->second.clip[0], imod->second.clip[1], imod->second.clip[2], imod->second.clip[3]);
						node->setColor(((color>>24)&0xff)/255.0f, ((color>>16)&0xff)/255.0f, ((color>>8)&0xff)/255.0f, (color&0xff)/255.0f);
						float mat[6];
						matrix2DComplex(mat, size[0], size[1], pivot[0], pivot[1], 1.0f, 1.0f, rot*0.01745329251994329576923690768489f, pos[0], pos[1]);
						node->setObjectMatrix(mat);
						if(frame.imageNode == NULL)
						{
							frame.imageNode = node; //一桢的第一个图块作为一桢的根节点
						}else
						{
							frame.imageNode->addChild(node);
						}
					}
					if(frame.imageNode == NULL)
						frame.imageNode = new Game2DImageNode(); //空frame
					paction->node->addChild(frame.imageNode);
				}else
				{
					paction->node->addChild(frame.imageNode->cloneBranch(false));
				}
				if(addblend)
					paction->node->getChild(paction->node->getChildCount()-1)->setDrawFlag(Game2DImageNode::DRAW_FLAG_BLEND_ADD | Game2DImageNode::DRAW_FLAG_BLEND_ADD_CHILD);
				if(framedata.size() > 0)
					time += framedata[framedata.size()-1].first; //累加桢持续时间，记录各桢的结束时间
				framedata.push_back(pair<float, int>(time, frame.info));
			}
			int numFrame = (int)framedata.size();
			paction->frame = new Animation2DAction::Frame[numFrame];
			for(int i=0; i<numFrame; i++)
			{
				paction->frame[i].endTime = framedata[i].first;
				paction->frame[i].info = (Animation2DFrameInfo*)framedata[i].second;
			}
		}
		m_numFrameInfo = (int)frameInfo.size();
		m_frameInfo = new Animation2DFrameInfo[m_numFrameInfo];
		for(int i=0; i<m_numFrameInfo; i++)
		{
			m_frameInfo[i].collisionRect = frameInfo[i].collisionRect;
			m_frameInfo[i].referencePoint = frameInfo[i].referencePoint;
		}
		for(ActionMap::iterator i = m_actionMap.begin(); i != m_actionMap.end(); ++i)
		{
			int numFrame = i->second.node->getChildCount();
			for(int j=0; j<numFrame; j++)
			{
				int iinfo = (int)i->second.frame[j].info;
				static Animation2DFrameInfo emptyInfo;
				i->second.frame[j].info = iinfo >=0 ? m_frameInfo + iinfo : &emptyInfo;
			}
		}
	}
	for(map<int, _image>::iterator i = imageMap.begin(); i != imageMap.end(); ++i)
	{
		if(i->second.tex != NULL)
			m_image.push_back(i->second.name);
	}
	return true;
}
void Animation2DResource::release(bool delay)
{
	for(ActionMap::iterator i = m_actionMap.begin(); i != m_actionMap.end(); ++i)
	{
		Game2DImageNode::deleteBranch(i->second.node, delay);
		delete i->second.frame;
	}
	for(size_t i=0; i<m_image.size(); i++)
		TextureManager::getSingleton()->release(m_image[i].c_str(), delay);
	m_actionMap.clear();
	delete[] m_frameInfo;
	m_numFrameInfo = 0;
}
Animation2D *Animation2DResource::createAnimation() const
{
	return new Animation2D(*this);
}
Animation2DAction Animation2DResource::createAction(const char *name) const
{
	assert(name != NULL);
	ActionMap::const_iterator iact = m_actionMap.find(name);
	if(iact == m_actionMap.end())
	{
		Animation2DAction ret = {NULL, NULL, false};
		return ret;
	}
	const Animation2DAction &action = iact->second;
	Animation2DAction ret = {action.frame, action.node->cloneBranch(false), action.loop};
	return ret;
}
Animation2DResource *Animation2DManager::Animation2DFactory::create(const char *name, bool delayLoad)
{
	Animation2DResource *res = new Animation2DResource(name);
	if(!delayLoad && !res->load(name, m_texDelay))
	{
		delete res;
		return NULL;
	}
	return res;
}
void Animation2DManager::Animation2DFactory::destroy(Animation2DResource *res)
{
	res->release(m_texDelay);
	delete res;
}
Animation2D *Animation2DManager::get(const char *name, bool delayLoad)
{
	Animation2DResource *res = m_resourceManager.get(name, delayLoad);
	return res != NULL ? res->createAnimation() : NULL;
}
void Animation2DManager::release(Animation2D *ani, bool delayDestroy)
{
	assert(ani != NULL);
	removeActiveAnimation(ani);
	m_resourceManager.release(ani->getName(), delayDestroy);
	delete ani;
}
void Animation2DManager::update(int timeStep)
{
	for(set<Animation2D *>::iterator i = m_activeSet.begin(); i!= m_activeSet.end();)
	{
		if((*i)->update(timeStep))
		{
			++i;
		}else
		{
			m_activeSet.erase(i++);
		}
	}
}
