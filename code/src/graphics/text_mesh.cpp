#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "material.hpp"

// helper thing for Mesh::FromFile() and TextMeshFromText() that will expand the vector so that it contains index if needed, then return vector.at(index)
template<typename T>
typename std::vector<T>::reference vectorAtExpanding(unsigned int index, std::vector<T>& vector) {
    if (vector.size() <= index) {
        vector.resize(index + 1, 6.66); // 6.66 is chosen so that uninitialized values are easy to see when debugging
    }
    return vector.at(index);
}

// (this function was in the header file)
void TextMeshFromText(std::string text, const Texture& font, const TextMeshCreateParams& params, const MeshVertexFormat& vertexFormat, std::vector<GLfloat>& vertices, std::vector<GLuint>& indices) {

    vertices.clear();
    indices.clear();

    // GLfloat windowWidth = 1; //GraphicsEngine::Get().window.width;
    // GLfloat windowHeight = 1; //GraphicsEngine::Get().window.height;

    // text wrapping
    if (params.wrapText) {

        // divide the text into words
        std::list<std::string> words; // list instead of vector so we can insert while iterating
        std::string currentLine;
        std::string currentWord;

        std::stringstream ss2(text);

        while (std::getline(ss2, currentLine, '\n')) {
            // std::getline only accepts one delimiter, so we do it twice, once for spaces and once for newlines

            std::stringstream ss(currentLine);

            while (std::getline(ss, currentWord, ' ')) {
                if (currentWord != "") {
                    words.push_back(currentWord);
                }

                words.push_back(" ");
            }

            words.push_back("\n");
        }

        // then, for each word, see if it fits on the current line. 
            //  If it's just a newline/space, duh.
            //  If it's bigger than maxLengthPerLine, split the word as needed. 
            //  Else, if it's bigger than  maxLengthPerLine - currentLineLegnth, put the word on the next line.
            //  Else, it fits on the current line, just append it directly to wrappedText (with a space)

        GLfloat currentLineLength = 0;
        GLfloat maxLengthPerLine = params.rightMargin - params.leftMargin;
        // std::cout << "Max length per line = " << maxLengthPerLine << ".\n";
        Assert(maxLengthPerLine > 0);
        std::string wrappedText;

        for (auto it = words.begin(); it != words.end(); it++) {
            std::string& word = *it;
            // std::cout << "Investigating word " << word << ".\n";
            if (word == "\n") {
                if (std::next(it) != words.end()) {
                    currentLineLength = 0;
                    wrappedText.push_back('\n');
                }

            }
            else if (word == " " && (std::next(it) == words.end() || *std::next(it) == "\n")) {
                // std::cout << "Skipped space.\n";
            }
            else {
                GLfloat wordLength = 0;
                unsigned int i = 0;
                for (const char& c : word) {
                    const auto& glyph = font.glyphs->at(c);
                    wordLength += glyph.advance;

                    if (wordLength > maxLengthPerLine && currentLineLength == 0) {
                        // split the string and add newline
                        // std::cout << "Splitting word " << word << " into " << word.substr(0, i) << " and " << word.substr(i, std::string::npos) << ".\n";

                        wrappedText += word.substr(0, i);
                        words.insert(std::next(it), word.substr(i, std::string::npos));
                        words.insert(std::next(it), "\n");

                        currentLineLength = 0;

                        goto dontDoOtherStuff;
                    }

                    i++;
                }
                // std::cout << "Length was " << wordLength << "!\n";

                if (wordLength > (maxLengthPerLine - currentLineLength)) {
                    wrappedText += "\n";

                    currentLineLength = 0;

                    it--;
                    goto dontDoOtherStuff;
                }

                wrappedText += word;
                currentLineLength += wordLength;

            dontDoOtherStuff:;
            }


        }

        text = wrappedText;
        // std::cout << "Produced text " << text << ".\n";
    }

    // find where each line starts, based on line width and horizontal alignment mode
    std::vector<GLfloat> lineStartOffsets;
    {
        GLfloat lineWidth = 0;
        for (auto it = text.begin(); it != text.end(); it++) {

            const char& c = *it;

            if (c == '\n') {

                switch (params.horizontalAlignMode) {
                case HorizontalAlignMode::Center:
                    lineStartOffsets.push_back(-lineWidth / 2.0f);
                    break;
                case HorizontalAlignMode::Left:
                    lineStartOffsets.push_back(-params.leftMargin);
                    break;
                case HorizontalAlignMode::Right:
                    lineStartOffsets.push_back(params.rightMargin - lineWidth);
                    break;
                }

                lineWidth = 0;
                continue;
            }

            const auto& glyph = font.glyphs->at(c);
            lineWidth += glyph.advance;
        }

        // if the last letter wasn't a newline, we still need to set the lineStartOffset for the last line
        if (lineWidth != 0) {
            switch (params.horizontalAlignMode) {
            case HorizontalAlignMode::Center:
                lineStartOffsets.push_back(-lineWidth / 2.0f);
                break;
            case HorizontalAlignMode::Left:
                lineStartOffsets.push_back(params.leftMargin);
                break;
            case HorizontalAlignMode::Right:
                lineStartOffsets.push_back(params.rightMargin - lineWidth);
                break;
            }
        }
    }


    // find the height and uppermost position of the text
    GLfloat topOfText = 0.0;
    GLfloat bottomOfText = 0.0;
    {

        for (const char& c : text) {
            auto& glyph = font.glyphs->at(c);
            if (c == '\n') {
                break;
            }
            else {
                topOfText = std::min(topOfText, -(GLfloat)glyph.height);
            }
        }

        unsigned int lineNumber = 0;
        for (const char& c : text) {
            auto& glyph = font.glyphs->at(c);

            if (c == '\n') {
                lineNumber += 1;
            }
            else {
                // std::cout << "Line number " << lineNumber << "char " << c << ", comparing current " << bottomOfText << " to new " << (GLfloat)(glyph.height - glyph.bearingY) + lineNumber * font.lineSpacing * params.lineHeightMultiplier << ".\n";
                bottomOfText = std::max(bottomOfText, (GLfloat)(int(glyph.height) - glyph.bearingY) + lineNumber * font.lineSpacing * params.lineHeightMultiplier);
            }
        }

        // topToBottom = (topOfText - bottomOfText);
    }

    GLfloat currentX = lineStartOffsets.at(0);

    // TODO: still room for improvement on vertical align
    GLfloat currentY = 0.0;
    switch (params.verticalAlignMode) {
    case VerticalAlignMode::Top:
        currentY = params.topMargin - topOfText;
        break;
    case VerticalAlignMode::Center:
        currentY = -topOfText + ((topOfText - bottomOfText) / 2.0f);
        break;
    case VerticalAlignMode::Bottom:
        currentY = params.bottomMargin;
        break;
    }

    // std::cout << "Top is " << topOfText << " top to bottom is " << topToBottom << " and starting at Y of " << currentY << ".\n";

    unsigned int lineNumber = 0;

    unsigned int vertexIndex = 0;
    unsigned int vertexSize = vertexFormat.GetNonInstancedVertexSize() / sizeof(GLfloat);

    for (const char& c : text) {
        if (c == '\n') {
            lineNumber += 1;
            currentY += font.lineSpacing * params.lineHeightMultiplier;
            currentX = lineStartOffsets.at(lineNumber);
            continue;
        }

        const auto& glyph = font.glyphs->at(c);

        // make sure the font actually has the letter we want (TODO give a default character instead)
        Assert(font.glyphs->count(c));

        // based on https://learnopengl.com/In-Practice/Text-Rendering 
        GLfloat characterX = currentX + glyph.bearingX;
        GLfloat width = glyph.width;
        GLfloat characterY = currentY - glyph.bearingY;
        GLfloat height = glyph.height;

        indices.push_back(vertexIndex + 2);
        indices.push_back(vertexIndex + 1);
        indices.push_back(vertexIndex);
        indices.push_back(vertexIndex + 3);
        indices.push_back(vertexIndex + 2);
        indices.push_back(vertexIndex);

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat), vertices) = (characterX);
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 1, vertices) = (characterY + height);
        if (vertexFormat.attributes.position->nFloats > 2) {
            // Assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat), vertices) = glyph.leftUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat) + 1, vertices) = glyph.bottomUv;
        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat), vertices) = (characterX);
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 1, vertices) = (characterY);
        if (vertexFormat.attributes.position->nFloats > 2) {
            // Assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat), vertices) = glyph.leftUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat) + 1, vertices) = glyph.topUv;

        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat), vertices) = (characterX + width);
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 1, vertices) = (characterY);
        if (vertexFormat.attributes.position->nFloats > 2) {
            // Assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat), vertices) = glyph.rightUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat) + 1, vertices) = glyph.topUv;
        vertexIndex += 1;

        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat), vertices) = (characterX + width);
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 1, vertices) = (characterY + height);
        if (vertexFormat.attributes.position->nFloats > 2) {
            // Assert(false);
            vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.position->offset / sizeof(GLfloat) + 2, vertices) = 0;
        }
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat), vertices) = glyph.rightUv;
        vectorAtExpanding(vertexIndex * vertexSize + vertexFormat.attributes.textureUV->offset / sizeof(GLfloat) + 1, vertices) = glyph.bottomUv;
        vertexIndex += 1;

        currentX += glyph.advance;
    }
}

std::pair<std::vector<float>, std::vector<unsigned int>> TextMeshProvider::GetMesh() const
{
    Assert(font);
    Assert(font->HasFontMap());

    std::vector<float> verts;
    std::vector<unsigned int> indices;
    TextMeshFromText(text, font->fontMapConstAccess.value(), textParams, meshParams.meshVertexFormat.value_or(MeshVertexFormat::DefaultGui()), verts, indices);
    return std::pair<std::vector<float>, std::vector<unsigned int>>();
}