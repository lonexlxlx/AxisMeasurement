param(
    [string]$SourcePath = (Join-Path $PSScriptRoot '..\docs\AxisMeasurement图形化测量操作与现场验收说明书.md'),
    [string]$DocxPath = (Join-Path $PSScriptRoot '..\docs\AxisMeasurement图形化测量操作与现场验收说明书.docx'),
    [string]$PdfPath = (Join-Path $PSScriptRoot '..\docs\AxisMeasurement图形化测量操作与现场验收说明书.pdf')
)

$ErrorActionPreference = 'Stop'
$word = New-Object -ComObject Word.Application
$word.Visible = $false
$word.DisplayAlerts = 0
$doc = $null

function Set-CellText {
    param($Cell, [string]$Text, [bool]$Header)
    $Cell.Range.Text = $Text.Trim()
    $Cell.VerticalAlignment = 1
    $Cell.Range.ParagraphFormat.SpaceAfter = 0
    $Cell.Range.ParagraphFormat.Alignment = if ($Header) { 1 } else { 0 }
    $Cell.Range.Font.NameFarEast = 'Microsoft YaHei'
    $Cell.Range.Font.Name = 'Arial'
    $Cell.Range.Font.Size = 9
    $Cell.Range.Font.Bold = if ($Header) { 1 } else { 0 }
    if ($Header) {
        $Cell.Shading.BackgroundPatternColor = 14277081
    }
}

try {
    $doc = $word.Documents.Add()
    $section = $doc.Sections.Item(1)
    $section.PageSetup.TopMargin = $word.CentimetersToPoints(1.8)
    $section.PageSetup.BottomMargin = $word.CentimetersToPoints(1.8)
    $section.PageSetup.LeftMargin = $word.CentimetersToPoints(2.0)
    $section.PageSetup.RightMargin = $word.CentimetersToPoints(2.0)

    $normal = $doc.Styles.Item(-1)
    $normal.Font.NameFarEast = 'Microsoft YaHei'
    $normal.Font.Name = 'Arial'
    $normal.Font.Size = 10.5
    $normal.ParagraphFormat.SpaceAfter = 5
    $normal.ParagraphFormat.LineSpacingRule = 1
    $normal.ParagraphFormat.LineSpacing = 16

    foreach ($styleId in @(-63, -2, -3)) {
        $style = $doc.Styles.Item($styleId)
        $style.Font.NameFarEast = 'Microsoft YaHei'
        $style.Font.Name = 'Arial'
        $style.Font.Color = 0
    }
    $doc.Styles.Item(-63).Font.Size = 24
    $doc.Styles.Item(-2).Font.Size = 16
    $doc.Styles.Item(-3).Font.Size = 12

    $lines = Get-Content -LiteralPath $SourcePath -Encoding UTF8
    $selection = $word.Selection
    $selection.SetRange(0, 0)
    $index = 0
    while ($index -lt $lines.Count) {
        $line = $lines[$index]
        if ([string]::IsNullOrWhiteSpace($line)) { $index++; continue }

        if ($line.StartsWith('|')) {
            $tableLines = New-Object System.Collections.Generic.List[string]
            while ($index -lt $lines.Count -and $lines[$index].StartsWith('|')) {
                $tableLines.Add($lines[$index])
                $index++
            }
            if ($tableLines.Count -ge 2) {
                $rows = @($tableLines | Where-Object { $_ -notmatch '^\|\s*[-:]+' })
                $columnCount = (($rows[0].Trim('|') -split '\|').Count)
                $selection.EndKey(6) | Out-Null
                $table = $doc.Tables.Add($selection.Range, $rows.Count, $columnCount)
                $table.Borders.Enable = 1
                $table.AllowAutoFit = $true
                $table.Rows.AllowBreakAcrossPages = 0
                $table.Rows.Item(1).HeadingFormat = -1
                for ($r = 0; $r -lt $rows.Count; $r++) {
                    $cells = $rows[$r].Trim('|') -split '\|'
                    for ($c = 0; $c -lt $columnCount; $c++) {
                        Set-CellText -Cell $table.Cell($r + 1, $c + 1) -Text $cells[$c] -Header ($r -eq 0)
                    }
                }
                $selection.SetRange($table.Range.End, $table.Range.End)
                $selection.TypeParagraph()
            }
            continue
        }

        $selection.EndKey(6) | Out-Null
        $selection.Style = $doc.Styles.Item(-1)
        $selection.Font.NameFarEast = 'Microsoft YaHei'
        $selection.Font.Name = 'Arial'
        $selection.Font.Size = 10.5
        $selection.Font.Bold = 0
        $selection.ParagraphFormat.Alignment = 0
        $selection.ParagraphFormat.KeepWithNext = 0
        if ($line.StartsWith('# ')) {
            $selection.Style = $doc.Styles.Item(-63)
            $selection.ParagraphFormat.Alignment = 1
            $selection.TypeText($line.Substring(2).Replace('`', ''))
        } elseif ($line.StartsWith('## ')) {
            $selection.Style = $doc.Styles.Item(-2)
            $selection.ParagraphFormat.KeepWithNext = -1
            $selection.TypeText($line.Substring(3).Replace('`', ''))
        } elseif ($line.StartsWith('### ')) {
            $selection.Style = $doc.Styles.Item(-3)
            $selection.ParagraphFormat.KeepWithNext = -1
            $selection.TypeText($line.Substring(4).Replace('`', ''))
        } elseif ($line.StartsWith('- ')) {
            $selection.TypeText([char]0x2022 + ' ' + $line.Substring(2).Replace('`', ''))
        } elseif ($line -match '^\d+\.\s+') {
            $selection.TypeText($line.Replace('`', ''))
        } elseif ($line.StartsWith('```')) {
            $index++
            $codeLines = New-Object System.Collections.Generic.List[string]
            while ($index -lt $lines.Count -and -not $lines[$index].StartsWith('```')) {
                $codeLines.Add($lines[$index])
                $index++
            }
            $selection.Font.NameFarEast = 'Microsoft YaHei'
            $selection.Font.Name = 'Consolas'
            $selection.Font.Size = 9
            $selection.Range.Shading.BackgroundPatternColor = 15790320
            $selection.TypeText($codeLines -join "`v")
        } else {
            $plainText = [string](($line -replace '  $', '').Replace('`', ''))
            $selection.TypeText($plainText)
        }
        $selection.TypeParagraph()
        $index++
    }

    $footer = $section.Footers.Item(1).Range
    $footer.Text = 'AxisMeasurement 图形化测量操作与现场验收说明书'
    $footer.Font.NameFarEast = 'Microsoft YaHei'
    $footer.Font.Size = 8
    $footer.ParagraphFormat.Alignment = 1

    $doc.Fields.Update() | Out-Null
    $doc.SaveAs2((Resolve-Path (Split-Path $DocxPath -Parent)).Path + '\' + (Split-Path $DocxPath -Leaf), 16)
    $doc.ExportAsFixedFormat((Resolve-Path (Split-Path $PdfPath -Parent)).Path + '\' + (Split-Path $PdfPath -Leaf), 17)
}
finally {
    if ($doc) { $doc.Close(0) }
    $word.Quit()
    if ($doc) { [void][Runtime.InteropServices.Marshal]::ReleaseComObject($doc) }
    [void][Runtime.InteropServices.Marshal]::ReleaseComObject($word)
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
