die "Usage: make_dicomrt.pl uids.txt contours.cxt\n" 
  unless $#ARGV >= 1;

use POSIX;
use File::Basename;

## -----------------------------------------------------------------
##  Global settings
## -----------------------------------------------------------------

## Dicomrt header strings
use lib dirname($0);
do "make_dicomrt_inc.pl" || die "Can't load include file: make_dicom_inc.pl";

$plastimatch_uid_prefix = "1.2.826.0.1.3680043.8.274.1";
$uids_fn = shift;
$cxt_fn = shift;
$dump_in_fn = "dump_in.txt";
$dcm_out_file = "dump.dcm";
$dump_out_fn = "dump_out.txt";

## -----------------------------------------------------------------
##  Structure file parsing routines
## -----------------------------------------------------------------
sub parse_cxt_format {
    my ($fn, $structures, $structure_names) = @_;

    ## This code was copied and edited from contour_points_to_cxt.pl.  
    ## It needs to be re-engineered into a common subroutine for both scripts.
    $series_CT_UID = "Unknown";  # For future work
    $same_study_set = 1;
    $have_roi_names = 0;

    ## Read header
    open CF, "<$fn" || die "CXT file not found: $fn\n";
    while (<CF>) {
	chomp;
	if (/SERIES_CT_UID/) {
	    ($junk, $series_CT_contour) = split;
	    if ($series_CT_contour ne $series_CT_UID) {
		print "SERIES_CT_UID_CT: $series_CT_UID\n";
		print "SERIES_CT_UID_CN: $series_CT_contour\n";
		warn "Warning: contours and ct are from different study sets\n";
		$same_study_set = 0;
	    }
	} else {
	    if (/ROI_NAMES/) {
		$have_roi_names = 1;
	    } elsif ($have_roi_names) {
		if (!/END_OF_ROI_NAMES/) {
		    ($structure,$color,$name) = split;
		    $structure_color_hash{$structure} = $color;
		    $structure_names_hash{$structure} = $name;
		}
	    }
	}
	last if /END_OF_ROI_NAMES/;
    }

    @roi_sort = sort { $a <=> $b } keys %structure_names_hash;
    while ($i = shift @roi_sort) {
	push @roi_names, $structure_names_hash{$i};
    }
    push @$structure_names, @roi_names;

    $old_struct = -1;
    while (<CF>) {
	($structure_no, 
	 $contour_thickness, 
	 $num_points, 
	 $slice_index, 
	 $uid_contour, 
	 $points) = split /\|/;

	if ($old_struct != $structure_no) {
	    if ($contours) {
		push @$structures, $contours;
		undef $contours;
	    }
	}
	$old_struct = $structure_no;
	push @{$contours}, $_;
    }
    close CF;
    if ($contours) {
	push @$structures, $contours;
	undef $contours;
    }
}

## -----------------------------------------------------------------
##  Load UID file
## -----------------------------------------------------------------
(-f $uids_fn) || die "Error: uid file \"$uids_fn\" not found\n";
open UIF, "<$uids_fn" || die "Can't open uid file \"$uids_fn\" for read\n";
while (<UIF>)
{
    next if /^#/;
    ($junk, $off_x, $off_y, $off_z, $nr_X, $nr_Y, $pixel_y, $pixel_x, $slice_uid, $study_img_uid, $series_img_uid, $for_img_uid) = split;
    $slice_UIDs{$off_z}=$slice_uid;
}
close UIF;
@slices = sort { $a <=> $b } keys %slice_UIDs;

## -----------------------------------------------------------------
##  Load contour files
## -----------------------------------------------------------------
@structures = ( );
@structure_names = ( );
parse_cxt_format ($cxt_fn, \@structures, \@structure_names);

if (0) {
for $structure (@structures) {
    for $contour (@{$structure}) {
	print "$contour\n";
	for $pt (@{$contour}) {
 	    print "  $pt\n";
 	}
    }
}
}

($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) 
  = localtime(time);
$date = sprintf("%04d%02d%02d",$year + 1900, $mon + 1, $mday);
$time = sprintf("%02d%02d%02d", $hour, $min, $sec);

($sysname, $nodename, $release, $version, $machine) = POSIX::uname();
#print "$sysname | $nodename | $release | $version | $machine\n";

$instance_creation_date = $date;
$instance_creation_time = $time;
$station_name = $nodename;
$software_version = "<EXPERIMENTAL>";

##########################################################################
## All of the following must be correct for GE AdvW to associate the 
## structures with the CT.
##########################################################################
## $patient_name = "Anonymous";
$patient_name = "NPC_panel";
$patient_id = "ANON42627";
## $patient_id = "0028";
$study_instance_uid = $study_img_uid;
## $series_instance_uid = "1.2.840.113619.2.55.1.1762853477.1996.1155908038.536.103";
$series_instance_uid = "2.16.840.1.114362.1.90609.1196125526343.847.103";
## $study_instance_uid = "1.2.840.113619.2.55.1.1762853477.1996.1155908038.536";
## $study_id = "9400";
$study_id = "ANON26726";
$patient_sex = "M";

$instance_creator_uid = $plastimatch_uid_prefix;
$sop_instance_uid = $plastimatch_uid_prefix . ".1.3026719";
$series_instance_uid = $plastimatch_uid_prefix . ".2.3026719";

$series_number = "103";
$instance_number = "1";
$structure_set_label = "Test";
$structure_set_name = "Test";
$structure_set_date = $date;
$structure_set_time = $time;

## Write dicomrt part 1
open OUT, ">$dump_in_fn";
printf OUT $head_103_part1,
  $instance_creation_date,
  $instance_creation_time,
  $instance_creator_uid,
  $sop_instance_uid,
  $station_name,
  $patient_name,
  $patient_id,
  $patient_sex,
##  $software_version,
  $study_instance_uid,
  $series_instance_uid,
  $study_id,
  $series_number,
  $instance_number
  ;

## Write dicomrt part 2 from image uid's
printf OUT $head_103_part2, 
  $structure_set_label,
  $structure_set_name,
  $structure_set_date,
  $structure_set_time,
  $for_img_uid,
  $study_img_uid,
  $series_img_uid;
for $i (0..$#slices) {
    printf OUT $item_103_part2, $slice_UIDs{$slices[$i]};
}
print OUT $foot_103_part2;

## Write dicomrt part 3 from contour file
print OUT $head_103_part3;
for $i (0..$#structures) {
  printf OUT $item_103_part3, 
    $i + 1, 
    $for_img_uid, 
    $structure_names[$i];
  ;
}
print OUT $foot_103_part3;

## Rebuild part 4 from contour file
print OUT $head_103_part4;
for $i (0..$#structures) {
    $structure = $structures[$i];
    $color = "255\\0\\0";
    printf OUT $subhead_103_part4, $color, $i + 1;
    $j = 1;
    for $contour (@{$structure}) {
	# Convert points to a string
#	$pts = "";
#	for $pt (@{$contour}) {
#	    ($x, $y, $z) = split ' ', $pt;
# 	    if ($z < 0 || $z > $#slices) {
#		printf "Warning: skipping contour with index %d ($i)\n", $z;
#		last;
#	    }
#	    $x = ($x * $pixel_x) + $off_x;
#	    $y = ($y * $pixel_y) + $off_y;
#	    $z_loc = $slices[$z];
#	    if ($pts) {
#		$pts = $pts . "\\$x\\$y\\$z_loc";
#	    } else {
#		$pts = "$x\\$y\\$z_loc";
#	    }
#	}
#	printf OUT $item_103_part4_without_ac, 
#	  $slice_UIDs{$z_loc}, $#{$contour}+1, $j, $pts;

	($structure_no, 
	 $contour_thickness, 
	 $num_points, 
	 $slice_index, 
	 $uid_contour, 
	 $points) = split /\|/, $contour;
	chomp ($points);

	printf OUT $item_103_part4_without_ac, 
	  $uid_contour, $num_points, $j, $points;

        $j = $j + 1;
    }
    printf OUT $subfoot_103_part4, $i + 1;
}
print OUT $foot_103_part4;

## Rebuild part 5 from contour file
print OUT $head_103_part5;
for $i (0..$#structures) {
    printf OUT $item_103_part5, 
      $i + 1,
      $i + 1,
      $structure_names[$i];
}
print OUT $foot_103_part5;
close OUT;

print `dump2dcm +E -e -g +l 524288 $dump_in_fn $dcm_out_file`;
#print `dcmdump +L +R 512 $dcm_out_file > $dump_out_fn`;

