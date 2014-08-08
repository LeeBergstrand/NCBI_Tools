#!/usr/local/bin/perl -w

################################################################################

use strict;

package Element;

use Scalar::Util "blessed";

my $new = '/home/klymenka/cvs/internal.new/asm-trace/OUTDIR/bin64/sra-stat';
my $old = '/panfs/traces01/trace_software/toolkit/centos64/bin/sra-stat.2.0.5';

sub setNewSraStat { $new = $_[0]; }
sub setOldSraStat { $old = $_[0]; }

sub new {
    my ($class, $cmd, $name, $i, $quick, $xml, $member, $src) = @_;

    my $self = {};
    $self->{'<CMD'} = $cmd;
    $self->{'<I'} = $i;
    $self->{'<MEMBER'} = $member ? 'off' : 'on';
    $self->{'<M'} = {};
    $self->{'<NAME'} = $name;
    $self->{'<QUICK'} = $quick;
    $self->{'<XML'} = $xml;

    if ($src) {
        if (blessed($src) && blessed($src) eq 'XML::LibXML::Element') {
            $self->{'<SRC'} = $src->toString();

            my @attributes = $src->attributes();
            $self->{$_->nodeName} = $_->value foreach (@attributes);
        } else {
            $self->{'<SRC'} = $src;

            chomp $src;

            my ($len, $mates, $bad, $filtered, @rest);
            ($self->{accession}, $self->{member_name}, $len, $mates,
             $bad, $filtered, @rest) = split(/\|/, $src);
            die @rest if (@rest);
            die $src unless ($len && $mates && $bad && $filtered);
            ($self->{spot_count}, $self->{base_count}, $self->{base_count_bio})
                = split(/:/, $len);
            ($self->{spot_count_mates}, $self->{base_count_bio_mates})
                = split(/:/, $mates);
            ($self->{spot_count_bad}, $self->{base_count_bio_bad})
                = split(/:/, $bad);
            ($self->{spot_count_filtered}, $self->{base_count_bio_filtered})
                = split(/:/, $filtered);
        }
    } else { $self->{'<SRC'} = '' }

    return bless $self, $class;
}

sub add {
    my ($self, $obj) = @_;

    $self->addMember($obj);
    $self->{'<SRC'} .= "$obj->{'<SRC'}";

    foreach my $name (keys %$obj) {
        next if ($name =~ /^</);
        next if ($name eq 'member_name');

        my $v = $obj->{$name};
        local $^W = 0;
        if (! ($v == 0 && $v ne '0')) { # add up all numeric attributes
            my $n = 0 + $v;
            my $s = '' . $n;
            if ($v eq $s)
            {   $self->{$name} += $v; }
        } elsif ($v eq '') {
            $self->{$name} += 0;
        }
    }
}

sub addMember {
    my ($self, $obj) = @_;

    my $m = {};
    $m->{$_} = $obj->{$_} foreach (keys %$obj);

    die unless (defined $obj->{member_name});
    my $member_name = $obj->{member_name};

    $self->{'<M'}->{$member_name} = $m;
}

sub allowed {
    my %allowed;

    ++$allowed{accession};
    ++$allowed{member_name};
    ++$allowed{read_length};

    my ($key) = @_;

    return 1 if ($allowed{$key});

    return 0;
}

################################################################################

sub equals {
    my ($self, $obj, $quick) = @_;
    my %key;
    ++$key{$_} foreach (keys %$obj);
    ++$key{$_} foreach (keys %$self);
    foreach (keys %key) {
        next if ($_ =~ /^</);
        # figure out why one run has an attribute and another doesn't
        if (!defined $obj->{$_}) {
            if ($quick && $obj->{'<QUICK'}) {
                next if (/^base_count_bio_bad$/);
                next if (/^base_count_bio_filtered$/);
                next if (/^base_count_bio_mates$/);
                next if (/^spot_count_bad$/);
                next if (/^spot_count_filtered$/);
                next if (/^spot_count_mates$/);
            }
            unless (allowed($_)) {
                if ($obj->{'<CMD'} =~ /^$old/ && $obj->{'<NAME'} eq 'Run' && $obj->{'<XML'}) { # TEST OLD <Run/>
                     die "$_ not defined in $self->{'<NAME'}" unless (/^base_count$/
                                                                   || /^base_count_bio$/
                                                                   || /^base_count_bio_bad$/
                                                                   || /^base_count_bio_filtered$/
                                                                   || /^base_count_bio_mates$/
                                                                   || /^spot_count$/
                                                                   || /^spot_count_bad$/
                                                                   || /^spot_count_filtered$/
                                                                   || /^spot_count_mates$/);
                    next;
                } elsif ($obj->{'<NAME'} eq 'Run' && $self->{'<NAME'} eq 'total'
                    && $obj->{'<CMD'} =~ m|^/panfs/traces01/trace_software/toolki|)
                {
                        next if (/^base_count$/);
                        next if (/^base_count_bio$/);
                        next if (/^base_count_bio_bad$/);
                        next if (/^base_count_bio_filtered$/);
                        next if (/^base_count_bio_mates$/);
                        next if (/^base_count_bio_mates$/);
                        next if (/^library$/);
                        next if (/^sample$/);
                        next if (/^spot_count_bad$/);
                        next if (/^spot_count_filtered$/);
                        next if (/^spot_count_mates$/);
                } elsif ($obj->{'<NAME'} eq 'total' && /^member_name$/) {
                    next;
                } elsif (/^cmp_base_count$/ && $self->{'<XML'} && ! $obj->{'<XML'}) {
                    # cmp_base_count is printed in XML but not in text mode
                    next;
                }

                my $msg = "$obj->{'<CMD'}: $_ not defined " .
                    "in $obj->{'<NAME'}($obj->{'<QUICK'}) " .
                    "but defined in $self->{'<CMD'}($self->{'<NAME'})\n" .
                    "$obj->{'<NAME'}($obj->{'<QUICK'}):" . # $obj->{'<SRC'} .
                    "\n";
                $msg .= $obj->toString();
                die $msg;
            }
        } elsif (!defined $self->{$_}) {
#####################################################################################################################
        # symmetric: figure out why another run has an attribute and one doesn't
        # could be combined with the previous check
            unless (allowed($_)) {
                if ($self->{'<CMD'} =~ /^$old/ && $self->{'<NAME'} eq 'Run' && $self->{'<XML'}) { # TEST OLD <Run/>
                     die "$_ not defined in $self->{'<NAME'}" unless (/^base_count$/
                                                                   || /^base_count_bio$/
                                                                   || /^base_count_bio_bad$/
                                                                   || /^base_count_bio_filtered$/
                                                                   || /^base_count_bio_mates$/
                                                                   || /^member_name$/
                                                                   || /^spot_count$/
                                                                   || /^spot_count_bad$/
                                                                   || /^spot_count_filtered$/
                                                                   || /^spot_count_mates$/
                                                                     );
                } elsif ($self->{'<CMD'} =~ /^$new/ && $self->{'<NAME'} eq 'Run' && $self->{'<XML'}) {
                     die "$_ not defined in $self->{'<NAME'}" unless (/^base_count_bio_bad$/
                                                                   || /^base_count_bio_filtered$/
                                                                   || /^base_count_bio_mates$/
                                                                   || /^member_name$/
                                                                   || /^spot_count_bad$/
                                                                   || /^spot_count_filtered$/
                                                                   || /^spot_count_mates$/
                                                                     );
                } elsif ($self->{'<QUICK'} && $self->{'<XML'}) {
                    die "$_ not defined in $self->{'<NAME'}" unless (/^base_count_bio_bad$/
                                                                  || /^base_count_bio_filtered$/
                                                                  || /^base_count_bio_mates$/
                                                                  || /^spot_count_bad$/
                                                                  || /^spot_count_filtered$/
                                                                  || /^spot_count_mates$/
                                                                    );
                } elsif (/^cmp_base_count$/ && !$self->{'<XML'} && $obj->{'<XML'}) {
                    # cmp_base_count is printed in XML but not in text mode
                } else {
                    die "$_ not defined in $self->{'<NAME'}";
                }
            }
        } elsif ($obj->{$_} ne $self->{$_}) {
            # figure out why run attribute values are different
            unless (($obj->{$_} eq '0' && $self->{$_} eq '')
                 || ($obj->{$_} eq '' && $self->{$_} eq '0'))
            {
                my ($min, $max);
                if ($obj->{$_} eq '' || $obj->{$_} < $self->{$_}) {
                    ($min, $max) = ($obj, $self);
                } else {
                    ($max, $min) = ($obj, $self);
                }
                if (($min->{$_} eq '' || $min->{$_} == 0) && $max->{$_} > 0) {
                    if (! $min->{'<XML'} && ! $max->{'<XML'} &&
                        $min->{'<QUICK'} && ! $max->{'<QUICK'})
                    {
                        die "$_: ($obj->{'<NAME'} != $self->{'<NAME'}): "
                            . "$obj->{$_} != $self->{$_}" unless (/^base_count_bio_bad$/
                                                               || /^base_count_bio_filtered$/
                                                               || /^base_count_bio_mates$/
                                                               || /^spot_count_bad$/
                                                               || /^spot_count_filtered$/
                                                               || /^spot_count_mates$/);
                    } elsif ($min->{'<QUICK'} && ! $max->{'<QUICK'}) { # && $max->{'<CMD'} =~ /^$new/) {
                        die "$_: ($obj->{'<NAME'} != $self->{'<NAME'}): "
                            . "$obj->{$_} != $self->{$_}" unless (/^base_count_bio_bad$/
                                                               || /^base_count_bio_filtered$/
                                                               || /^base_count_bio_mates$/
                                                               || /^spot_count_bad$/
                                                               || /^spot_count_filtered$/
                                                               || /^spot_count_mates$/);
                    } else {
                        die "$_: ($obj->{'<NAME'} != $self->{'<NAME'}): "
                            . "$obj->{$_} != $self->{$_}";
                    }
                }
                else {
                    die "$_: ($obj->{'<CMD'}/$obj->{'<NAME'} "
                        . "!= $self->{'<CMD'}/$self->{'<NAME'}): "
                        . "$obj->{$_} != $self->{$_}";
                }
            }
        }
    }
    $self->membersEqual($obj);
}
################################################################################
sub membersEqual {
    my ($self, $obj) = @_;
    my $sMembers = $self->{'<M'};
    my $oMembers = $obj->{'<M'};

    # figure out why number of members is different
    if (keys %$sMembers != keys %$oMembers) {
        if (keys %$sMembers == 0) {
            if ($self->{'<NAME'} eq 'sra-stat') {
                return 1;
            } elsif ($self->{'<MEMBER'} eq 'off') {
                return 1;
            } elsif (keys %$oMembers == 1 && $obj->{'<NAME'} eq 'total'
                && $self->{'<XML'} && $self->{'<NAME'} eq 'Run'
                && ! $obj->{'<XML'})
            {
                return 1;
            } elsif (keys %$oMembers == 1
                && $obj->{'<CMD'} =~ /^$old/ && $self->{'<CMD'} =~ /^$new/
                && $obj->{'<XML'} && $self->{'<XML'}
                && (keys %$oMembers)[0] eq '')
            {
                return 1;
            }
            die "$self->{'<CMD'}($self->{'<NAME'}) $obj->{'<CMD'}\n"
                . (0 + keys %$sMembers) . " " . (0 + keys %$oMembers);
        } elsif (keys %$oMembers == 0) {
            if ($obj->{'<NAME'} eq 'sra-stat') {
                return 1;
            } elsif ($obj->{'<MEMBER'} eq 'off') {
                return 1;
            } elsif (keys %$sMembers == 1
                && $self->{'<NAME'} eq 'total' && ! $self->{'<XML'}
                && $obj->{'<NAME'} eq 'Run' && $obj->{'<XML'})
            {
                return 1;
            } elsif (keys %$sMembers == 1
                && $self->{'<CMD'} =~ /^$old/ && $obj->{'<CMD'} =~ /^$new/
                && $obj->{'<XML'} && $self->{'<XML'}
                && (keys %$sMembers)[0] eq '')
            {
                return 1;
            }
            die "$self->{'<CMD'}($self->{'<NAME'}) " .
                "$obj->{'<CMD'}($obj->{'<NAME'})\n"
                . (0 + keys %$sMembers) . " " . (0 + keys %$oMembers);
        }
    }

    # compare all members
    foreach my $member (keys %$sMembers) {
        my $sKey = $member;
        my $oKey = $member;
        unless ($oMembers->{$member}) {
            if ($member eq '') {
                my $key = 'default';
                die if ($sMembers->{$key});
                $member = $key;
                $oKey = $key;
                die unless ($oMembers->{$member});
            } elsif ($member eq 'default') {
                my $key = '';
                die if ($sMembers->{$key});
                $member = $key;
                $oKey = $key;
                die unless ($oMembers->{$member});
            } else {
                print "$self->{'<CMD'} :\n";
                print keys %$sMembers;
                print "\n";
                print "$obj->{'<CMD'} :\n";
                print keys %$oMembers;
                die "'$member'";
            }
        }
        my $oMember = $oMembers->{$oKey};
        my $sMember = $sMembers->{$sKey};
#####################################################################################################################################
        unless (keys(%$oMember) == keys(%$sMember)) {
            # figure out why member attributes are different
            my ($max, $min, $pMax, $pMin);
            if (keys(%$oMember) > keys(%$sMember)) {
                ($max, $min, $pMax, $pMin) = ($oMember, $sMember, $obj, $self);
            } else {
                ($min, $max, $pMin, $pMax) = ($oMember, $sMember, $obj, $self);
            }
            my @keysMin = qw(<CMD <I <M <MEMBER <NAME <QUICK <SRC <XML base_count base_count_bio member_name spot_count);
            my @keysMax = @keysMin;
            push @keysMax, 'accession' unless ($pMax->{'<XML'});
            push @keysMin, 'accession' unless ($pMin->{'<XML'});
            push @keysMin, 'base_count_bio_bad' if (defined $min->{base_count_bio_bad});
            push @keysMin, 'base_count_bio_filtered' if (defined $min->{base_count_bio_filtered});
            push @keysMin, 'base_count_bio_mates' if (defined $min->{base_count_bio_mates});
            push @keysMin, 'cmp_base_count' if ($min->{cmp_base_count});
            push @keysMin, 'library' if ($min->{library});
            push @keysMin, 'sample' if ($min->{sample});
            push @keysMin, 'spot_count_bad' if (defined $min->{spot_count_bad});
            push @keysMin, 'spot_count_filtered' if (defined $min->{spot_count_filtered});
            push @keysMin, 'spot_count_mates' if (defined $min->{spot_count_mates});
push @keysMax,qw(base_count_bio_bad base_count_bio_filtered base_count_bio_mates spot_count_bad spot_count_mates spot_count_filtered)
                if ((!$pMax->{'<XML'}) || ($pMax->{'<XML'} && !$pMax->{'<QUICK'}));
            die "$pMin->{'<CMD'}\n" . join(' ', sort keys(%$min)) . "\n" . join(' ', sort @keysMin)
                unless (($#keysMin + 1) == (0 + keys %$min) || ($#keysMin + 0) == (0 + keys %$min));
            push @keysMax, 'cmp_base_count' if ($max->{cmp_base_count});
            push @keysMax, 'library' if ($max->{library});
            push @keysMax, 'sample' if ($max->{sample});
            die "$pMax->{'<CMD'}\n" . join(' ', keys(%$max)) . "\n" . join(' ', @keysMax) if (($#keysMax + 1) != (0 + keys %$max));
            foreach (@keysMin) {
                die "$_\n" . join(' ', keys(%$min)) unless (defined $min->{$_});
                unless (defined $max->{$_}) {
                    die $_ unless (/^accession$/ || /^cmp_base_count$/ || /^library$/ || /^sample$/);
                    next;
                }
                next if (/^</);
                unless ($min->{$_} eq $max->{$_}) {
                    if ($_ eq 'member_name') {
                        unless (($min->{$_} eq '' && $max->{$_} eq 'default')
                             || ($max->{$_} eq '' && $min->{$_} eq 'default'))
                        { die "$_ '$min->{$_}' '$max->{$_}'" ; }
                    } elsif (/^base_count_bio_bad$/
                          || /^base_count_bio_filtered$/
                          || /^base_count_bio_mates$/
                          || /^spot_count_bad$/
                          || /^spot_count_filtered$/
                          || /^spot_count_mates$/
                        )
                    {
                        unless (($min->{$_} eq '' && $max->{$_} eq '0')
                             || ($max->{$_} eq '' && $min->{$_} eq '0'))
                        {
                            unless ($pMax->{'<QUICK'} && ! $pMin->{'<QUICK'} && $max->{$_} eq '' && $min->{$_} > 0) {
                                unless ($pMin->{'<QUICK'} && ! $pMax->{'<QUICK'} && $min->{$_} eq '' && $max->{$_} > 0 &&
                                    (/^base_count_bio_bad$/ || /^base_count_bio_mates$/ || /^spot_count_bad$/ || /^spot_count_mates$/))
                                {   die "$_ '$min->{$_}' '$max->{$_}'\n$min->{'<CMD'}' '$max->{'<CMD'}" ; }
                            }
                        }
                    } else {
                        die "$_ '$min->{$_}' '$max->{$_}'" ;
                    } 
                }
            }
        }
    }
#            die "$pMax->{'<CMD'}\n$pMin->{'<CMD'}\n" .
 #               join(' ', keys(%$max)) . "\n" . join(' ', keys(%$min));
  #              die "" . (keys(%$max) - keys(%$min)) . "\n" .
   #                 join(' ', keys(%$max)) . "\n" . join(' ', keys(%$min));
#        die "'$member' $obj->{'<XML'} $self->{'<XML'}\n" .
 #           join(' ', keys(%$oMember)) . "\n" . join(' ', keys(%$sMember))
  #              unless (keys(%$oMember) == keys(%$sMember));
}

sub toString {
    my $self = shift;

    my $s = "CMD='$self->{'<CMD'}'";
    $s .= " NAME=$self->{'<NAME'}";
#   $s .= \nATTRS =";
#   $s .= " $_" foreach (keys %$self);

    return $s;
}

1
