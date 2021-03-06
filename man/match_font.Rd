% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/match_font.R
\name{match_font}
\alias{match_font}
\title{Find a system font by name and style}
\usage{
match_font(family, italic = FALSE, bold = FALSE)
}
\arguments{
\item{family}{The name of the font family}

\item{italic, bold}{logicals indicating the font style}
}
\value{
A list containing the path locating the font file and the 0-based
index of the font in the file.
}
\description{
This function locates the font file (and index) best matching a name and
optional style (italic/bold). A font file will be returned even if a match
isn't found, but it is not necessarily similar to the requested family and
it should not be relied on for font substitution. The aliases \code{"sans"},
\code{"serif"}, and \code{"mono"} match to the system default sans-serif, serif, and
mono fonts respectively (\code{""} is equivalent to \code{"sans"}).
}
\examples{
# Get the system default sans-serif font in italic
match_font('sans', italic = TRUE)

}
