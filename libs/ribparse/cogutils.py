import xml.etree.ElementTree as etree
import re
import exceptions

#-------------------------------------------------------------------------------
# XML functions

def patchEtreeElementClass():
    '''Monky patch the etree interface to include some handy methods'''
    # The element class type is left undefined in the docs, so we use the
    # Element() factory function to get an instance & then retrive the exact
    # element class for modification :-)
    elementClass = etree.Element('dummy').__class__
    def haschild(element, name):
        return element.find(name) is not None
    elementClass.haschild = haschild
#    def elementGetAttr(element, name):
#        if name.startswith('__'):
#            raise AttributeError('special method?')
#        elements = element.findall(name)
#        if len(elements) == 0:
#            raise AttributeError('element "%s" has no child named "%s"'
#                                 % (element.tag, name))
#        elif len(elements) == 1:
#            return elements[0]
#        else:
#            return elements
#    elementClass.__getattr__ = elementGetAttr

# WARNING: Dynamically patching the Element class is a bit of an ugly hack, but
# how else can we get methods attached to element objects?
#
# It does make the resulting code much nicer to read ;-)
patchEtreeElementClass()

def parseXmlTree(fileName):
    return etree.ElementTree().parse(fileName)


#-------------------------------------------------------------------------------
# Code formatting functions
def commentBanner(str, width=70, fillchar='='):
    comment = '// '
    return comment + (' ' + str + ' ').center(width - len(comment), fillchar)

def wrapDecl(str, maxWidth, wrapIndent=None):
    '''Wrap a one-line declaration onto multiple lines.'''
    splits = re.split(', *', str);
    if len(splits) == 1:
        return str
    line = splits.pop(0)
    if wrapIndent is None:
        indent = (line.find('(') + 1)*' '
    else:
        indent = ' ' * wrapIndent
    result = []
    while len(line) != 0:
        while len(splits) > 0 and len(line) + 3 + len(splits[0]) <= maxWidth:
            line += ', '
            line += splits.pop(0)
        result += [line]
        if len(splits) == 0:
            line = ''
        else:
            line = indent + splits.pop(0)
    return ',\n'.join(result)



#-------------------------------------------------------------------------------
# Code generation functions with etree objects as arguments.
#
def ribArgs(argList):
    return filter(lambda arg: not arg.haschild('RibValue'), argList)

def cName(procXml):
    if procXml.haschild('CName'):
        return procXml.findtext('CName')
    else:
        return 'Ri' + procXml.findtext('Name')

def formalArg(arg):
    type = arg.findtext('Type')
    name = arg.findtext('Name')
    if type is None:
        return name
    if type.endswith('Array'):
        return 'const %sArray& %s' % (type[2:-5], name)
    else:
        type = re.sub(r'Rt(Token|String|Color|Point(?!er)|Matrix|Bound|Basis)',
                      r'RtConst\1', type)
    return '%s %s' % (type, name)

def riCxxMethodDecl(procXml, className=None):
    args = [formalArg(arg) for arg in ribArgs(procXml.findall('Arguments/Argument'))]
    if procXml.haschild('Arguments/ParamList'):
        args += ['const ParamList& pList']
    procName = procXml.findtext('Name')
    if className is not None:
        procName = className + '::' + procName
    return '%s %s(%s)' % (procXml.findtext('ReturnType'), procName, ', '.join(args))

