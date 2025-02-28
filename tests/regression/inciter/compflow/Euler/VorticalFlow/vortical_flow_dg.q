# vim: filetype=sh:
# This is a comment
# Keywords are case-sensitive

title "Vortical flow"

inciter

  nstep 100   # Max number of time steps
  dt   1.0e-4 # Time step size
  ttyi 5      # TTY output interval
  scheme dg

  compflow

    physics euler
    problem vortical_flow
    depvar u

    alpha 0.1
    beta 1.0
    p0 10.0

    material
      gamma 1.66666666666667 end # =5/3 ratio of specific heats
    end

    bc_dirichlet
      sideset 1 2 3 4 5 6 end
    end

  end

  diagnostics
    interval  1
    format    scientific
    error l2
  end

  field_output
    var
      elem
      analytic
      density "density_numerical"
      x-velocity "x-velocity_numerical"
      y-velocity "y-velocity_numerical"
      z-velocity "z-velocity_numerical"
      specific_total_energy "specific_total_energy_numerical"
      pressure "pressure_numerical"
    end
    interval 10
  end

end
