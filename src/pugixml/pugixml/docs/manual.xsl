<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:template name="navbar.section">
		<xsl:param name="name" select="/foo" />
        <xsl:param name="targets" select="key('id',$name)"/>
        <xsl:param name="target" select="$targets[1]"/>
		<xsl:param name="text" select="normalize-space($target/title)" />

		<xsl:choose>
			<xsl:when test="@id != $name">
				<a>
				<xsl:attribute name="href">
					<xsl:call-template name="href.target">
						<xsl:with-param name="object" select="$target"/>
					</xsl:call-template>
				</xsl:attribute>
				<xsl:value-of select="$text" />
				</a>
			</xsl:when>
			<xsl:otherwise>
				<b>
				<xsl:value-of select="$text" />
				</b>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template name="navbar.contents">
		<a href="http://pugixml.org/"><xsl:value-of select="/*/title" /></a> manual |
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual'" /><xsl:with-param name="text" select="'Overview'" /></xsl:call-template> |
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.install'" /></xsl:call-template> |
		Document:
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.dom'" /><xsl:with-param name="text" select="'Object model'" /></xsl:call-template>
		<xsl:text disable-output-escaping="yes"> &amp;middot; </xsl:text>
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.loading'" /><xsl:with-param name="text" select="'Loading'" /></xsl:call-template>
		<xsl:text disable-output-escaping="yes"> &amp;middot; </xsl:text>
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.access'" /><xsl:with-param name="text" select="'Accessing'" /></xsl:call-template>
		<xsl:text disable-output-escaping="yes"> &amp;middot; </xsl:text>
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.modify'" /><xsl:with-param name="text" select="'Modifying'" /></xsl:call-template>
		<xsl:text disable-output-escaping="yes"> &amp;middot; </xsl:text>
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.saving'" /><xsl:with-param name="text" select="'Saving'" /></xsl:call-template> |
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.xpath'" /></xsl:call-template> |
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.apiref'" /></xsl:call-template> |
		<xsl:call-template name="navbar.section"><xsl:with-param name="name" select="'manual.toc'" /></xsl:call-template>
	</xsl:template>

	<xsl:template name="header.navigation">
		<xsl:param name="prev" select="/foo"/>
		<xsl:param name="next" select="/foo"/>
		<xsl:param name="nav.context"/>

		<table width="100%"><tr>
		<td>
			<xsl:call-template name="navbar.contents" />
		</td>
		<td width="*" align="right">
			<xsl:call-template name="navbar.spirit">
				<xsl:with-param name="prev" select="$prev"/>
				<xsl:with-param name="next" select="$next"/>
				<xsl:with-param name="nav.context" select="$nav.context"/>
			</xsl:call-template>
		</td>

		</tr></table>
		<hr/>
	</xsl:template>

	<xsl:template name="footer.navigation">
		<xsl:param name="prev" select="/foo"/>
		<xsl:param name="next" select="/foo"/>
		<xsl:param name="nav.context"/>

		<hr/>
		<table width="100%"><tr>
		<td>
			<xsl:call-template name="navbar.contents" />
		</td>
		<td width="*" align="right">
			<xsl:call-template name="navbar.spirit">
				<xsl:with-param name="prev" select="$prev"/>
				<xsl:with-param name="next" select="$next"/>
				<xsl:with-param name="nav.context" select="$nav.context"/>
			</xsl:call-template>
		</td>

		</tr></table>
	</xsl:template>

	<xsl:template match="section[@id='manual.toc']/para[normalize-space(text())='toc-placeholder']">
		<xsl:call-template name="make.toc">
			<xsl:with-param name="toc-context" select="." />
			<xsl:with-param name="toc.title.p" select="false()" />
			<xsl:with-param name="nodes" select="/book/section" />
		</xsl:call-template>
	</xsl:template>

    <xsl:template name="book.titlepage" />

	<xsl:template name="anchor">
		<xsl:param name="node" select="." />
		<xsl:param name="conditional" select="1" />
		<xsl:variable name="id">
			<xsl:call-template name="object.id">
				<xsl:with-param name="object" select="$node"/>
			</xsl:call-template>
		</xsl:variable>
		<xsl:choose>
			<xsl:when test="$node/@id or $node/@xml:id">
				<a name="{$id}"/>
			</xsl:when>
			<xsl:when test="$conditional = 0 and local-name($node) != 'bridgehead'">
				<xsl:message terminate="yes">
				  <xsl:text>ERROR: Autogenerated id detected for element </xsl:text><xsl:value-of select="local-name($node)" />
				</xsl:message>
			</xsl:when>
		</xsl:choose>
	</xsl:template>
</xsl:stylesheet>  
