/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2010 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief Definition of the class Texture.
 *
 *   @author Lionel Heng <hengli@student.ethz.ch>
 *
 */

#include <osg/LineWidth>

#include "Texture.h"

Texture::Texture()
    : _is3D(false)
{
    texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    GLuint id;
    glGenTextures(1, &id);
    t->setID(id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

const QString&
Texture::getSourceURL(void) const
{
    return sourceURL;
}

void
Texture::setId(unsigned int _id)
{
    id = _id;
}

void
Texture::sync(const WebImagePtr& image)
{
    state = static_cast<State>(image->getState());

    if (image->getState() != WebImage::UNINITIALIZED &&
        (sourceURL != image->getSourceURL() ||
         _is3D != image->is3D()))
    {
        sourceURL = image->getSourceURL();
        _is3D = image->is3D();
    }

    if (image->getState() == WebImage::READY && image->getSyncFlag())
    {
        image->setSyncFlag(false);

        if (image->getWidth() != imageWidth ||
            image->getHeight() != imageHeight)
        {
            imageWidth = image->getWidth();
            textureWidth = 32;
            while (textureWidth < imageWidth)
            {
                textureWidth *= 2;
            }
            imageHeight = image->getHeight();
            textureHeight = 32;
            while (textureHeight < imageHeight)
            {
                textureHeight *= 2;
            }

            maxU = static_cast<double>(imageWidth)
                   / static_cast<double>(textureWidth);
            maxV = static_cast<double>(imageHeight)
                   / static_cast<double>(textureHeight);

            osg::ref_ptr<osg::Image> image;
            image->setImage(textureWidth, textureHeight, 8, 3, GL_RGBA, GL_UNSIGNED_BYTES, NULL, osg::Image::USE_NEW_DELETE);

            texture2D->
            glBindTexture(GL_TEXTURE_2D, id);
            glTexImage2D(GL_TEXTURE_2D, 0, 3, textureWidth, textureHeight,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }

        glBindTexture(GL_TEXTURE_2D, id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageWidth, imageHeight,
                        GL_RGBA, GL_UNSIGNED_BYTE, image->getImageData());

        heightModel = image->getHeightModel();
    }
}

osg::ref_ptr<osg::Geometry>
Texture::draw(float x1, float y1, float x2, float y2,
              bool smoothInterpolation) const
{
    return draw(x1, y1, x2, y1, x2, y2, x1, y2, smoothInterpolation);
}

osg::ref_ptr<osg::Geometry>
Texture::draw(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              bool smoothInterpolation) const
{
    osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry);
    osg::ref_ptr<osg::StateSet> stateset(new osg::StateSet);

    if (state == REQUESTED)
    {
        osg::ref_ptr<osg::Vec2Array> vertices(new osg::Vec2Array);
        vertices->push_back(osg::Vec2(x1, y1));
        vertices->push_back(osg::Vec2(x2, y2));
        vertices->push_back(osg::Vec2(x3, y3));
        vertices->push_back(osg::Vec2(x4, y4));

        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES,
                                                      0, vertices->size()));

        geometry->setVertexArray(vertices);

        osg::ref_ptr<osg::Vec4Array> color(new osg::Vec4Array);
        color->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        geometry->setColorArray(color);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        
        return geometry;
    }

    stateset->setTextureAttributeAndModes(id, texture2D);

    float dx, dy;
    if (smoothInterpolation)
    {
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        dx = 1.0f / (2.0f * textureWidth);
        dy = 1.0f / (2.0f * textureHeight);
    }
    else
    {
        texture2D->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        texture2D->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        dx = 0.0f;
        dy = 0.0f;
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    if (!_is3D)
    {
        osg::ref_ptr<osg::Vec2Array> tc = new osg::Vec2Array;

        geometry->setTexCoordArray(id, tc);
        tc->push_back(osg::Vec2(dx, maxV - dy));
        tc->push_back(osg::Vec2(maxU - dx, maxV - dy));
        tc->push_back(osg::Vec2(maxU - dx, dy));
        tc->push_back(osg::Vec2(dx, dy));

        glBegin(GL_QUADS);
        glTexCoord2f(dx, maxV - dy);
        glVertex3f(x1, y1, 0.0f);
        glTexCoord2f(maxU - dx, maxV - dy);
        glVertex3f(x2, y2, 0.0f);
        glTexCoord2f(maxU - dx, dy);
        glVertex3f(x3, y3, 0.0f);
        glTexCoord2f(dx, dy);
        glVertex3f(x4, y4, 0.0f);
        glEnd();
    }
    else
    {
        float scaleX = 1.0f / static_cast<float>(heightModel.size() - 1);

        for (int32_t i = 0; i < heightModel.size() - 1; ++i)
        {
            float scaleI = scaleX * static_cast<float>(i);

            float scaleY =
                    1.0f / static_cast<float>(heightModel[i].size() - 1);

            float x1i = x1 + scaleI * (x4 - x1);
            float x1f = x2 + scaleI * (x3 - x2);
            float x2i = x1i + scaleX * (x4 - x1);
            float x2f = x1f + scaleX * (x3 - x2);

            for (int32_t j = 0; j < heightModel[i].size() - 1; ++j)
            {
                float scaleJ = scaleY * static_cast<float>(j);

                float y1i = y1 + scaleJ * (y2 - y1);
                float y1f = y4 + scaleJ * (y3 - y4);
                float y2i = y1i + scaleY * (y2 - y1);
                float y2f = y1f + scaleY * (y3 - y4);

                float nx1 = x1i + scaleJ * (x1f - x1i);
                float nx2 = x1i + (scaleJ + scaleY) * (x1f - x1i);
                float nx3 = x2i + (scaleJ + scaleY) * (x2f - x2i);
                float nx4  = x2i + scaleJ * (x2f - x2i);
                float ny1 = y1i + scaleI * (y1f - y1i);
                float ny2 = y2i + scaleI * (y2f - y2i);
                float ny3 = y2i + (scaleI + scaleX) * (y2f - y2i);
                float ny4 = y1i + (scaleI + scaleX) * (y1f - y1i);

                glBegin(GL_QUADS);
                glTexCoord2f(dx + scaleJ * (maxU - dx * 2.0f),
                             dy + (1.0f - scaleI) * (maxV - dy * 2.0f));
                glVertex3f(nx1, ny1, -static_cast<float>(heightModel[i][j]));
                glTexCoord2f(dx + (scaleJ + scaleY) * (maxU - dx * 2.0f),
                             dy + (1.0f - scaleI) * (maxV - dy * 2.0f));
                glVertex3f(nx2, ny2, -static_cast<float>(heightModel[i][j + 1]));
                glTexCoord2f(dx + (scaleJ + scaleY) * (maxU - dx * 2.0f),
                             dy + (1.0f - scaleI - scaleX) * (maxV - dy * 2.0f));
                glVertex3f(nx3, ny3, -static_cast<float>(heightModel[i + 1][j + 1]));
                glTexCoord2f(dx + scaleJ * (maxU - dx * 2.0f),
                             dy + (1.0f - scaleI - scaleX) * (maxV - dy * 2.0f));
                glVertex3f(nx4, ny4, -static_cast<float>(heightModel[i + 1][j]));

                glEnd();
            }
        }
    }
}

bool
Texture::is3D(void) const
{
    return _is3D;
}
