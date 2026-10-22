package api.project.repository;

import api.project.model.Project;
import io.quarkus.hibernate.orm.panache.PanacheRepositoryBase;
import jakarta.enterprise.context.ApplicationScoped;

import java.util.UUID;

@ApplicationScoped
public class ProjectRepository implements PanacheRepositoryBase<Project, UUID> {
}